#include "process_runner.h"

#include <uv.h>

typedef struct {
    uv_loop_t loop;
    process_job_t *jobs;
    word count;
    word next;
    uint32 active;
    uint32 max_parallel;
    bool failed;
} process_runner_t;

typedef struct {
    uv_process_t process;
    uv_pipe_t out_pipe;
    uv_pipe_t err_pipe;

    stream_t out_stream;
    stream_t err_stream;

    process_runner_t *runner;
    process_job_t *job;
    int spawn_error;
    int open_handles;
} process_context_t;

static void process_runner_spawn_next(process_runner_t *runner);

static void process_runner_print_stream(stream_t *stream, bool is_error) {
    if (stream_get_position(stream) == 0)
        return;

    stream_write(stream, (const byte *)"", 1);
    fputs((const char *)stream->data, is_error ? stderr : stdout);
}

/* Chamado quando os 3 handles (processo e os 2 pipes) foram fechados:
   só aqui a saída está completa e a memória pode ser liberada. */
static void process_runner_on_handle_close(uv_handle_t *handle) {
    process_context_t *ctx = (process_context_t *)handle->data;
    if (--ctx->open_handles > 0)
        return;

    process_job_t *job = ctx->job;
    if (job->capture) {
        stream_write(&ctx->out_stream, (const byte *)"", 1);
        job->output = (char *)ctx->out_stream.data;
        ctx->out_stream = (stream_t){ 0 }; // a posse do buffer passa para o job
    } else {
        process_runner_print_stream(&ctx->out_stream, false);
        process_runner_print_stream(&ctx->err_stream, true);

        if (ctx->spawn_error)
            log_error("Falha ao executar '%s': %s", job->cmd->args[0], uv_strerror(ctx->spawn_error));
        else if (job->exit_status != 0) {
            const char *what = job->label ? job->label : job->cmd->args[0];
            while (*what == ' ')
                what++;
            log_error("%s: falhou (código %lld)", what, (long long)job->exit_status);
        }
    }

    stream_clear(&ctx->out_stream);
    stream_clear(&ctx->err_stream);
    free(ctx);
}

static void process_runner_alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = malloc(suggested_size);
    buf->len = buf->base ? suggested_size : 0;
}

static void process_runner_read(uv_stream_t *pipe, ssize_t nread, const uv_buf_t *buf, stream_t *stream) {
    if (nread > 0)
        stream_write(stream, (const byte *)buf->base, nread);
    else if (nread < 0 && !uv_is_closing((uv_handle_t *)pipe))
        uv_close((uv_handle_t *)pipe, process_runner_on_handle_close);

    free(buf->base);
}

static void process_runner_read_out(uv_stream_t *pipe, ssize_t nread, const uv_buf_t *buf) {
    process_context_t *ctx = (process_context_t *)pipe->data;
    process_runner_read(pipe, nread, buf, &ctx->out_stream);
}

static void process_runner_read_err(uv_stream_t *pipe, ssize_t nread, const uv_buf_t *buf) {
    process_context_t *ctx = (process_context_t *)pipe->data;
    process_runner_read(pipe, nread, buf, &ctx->err_stream);
}

static void process_runner_on_exit(uv_process_t *process, int64_t exit_status, int term_signal) {
    process_context_t *ctx = (process_context_t *)process->data;
    process_runner_t *runner = ctx->runner;

    ctx->job->exit_status = term_signal ? 128 + term_signal : exit_status;
    if (ctx->job->exit_status != 0)
        runner->failed = true;

    uv_close((uv_handle_t *)process, process_runner_on_handle_close);

    runner->active--;
    process_runner_spawn_next(runner);
}

static void process_runner_spawn_next(process_runner_t *runner) {
    while (runner->active < runner->max_parallel && runner->next < runner->count) {
        process_job_t *job = &runner->jobs[runner->next++];

        process_context_t *ctx = (process_context_t *)calloc(1, sizeof(process_context_t));
        if (!ctx) {
            LOG_FATAL_NOT_ENOUGH_MEMORY();
            runner->failed = true;
            return;
        }

        ctx->runner = runner;
        ctx->job = job;
        ctx->process.data = ctx;
        ctx->out_pipe.data = ctx;
        ctx->err_pipe.data = ctx;
        ctx->open_handles = 3; // processo, stdout e stderr

        stream_init(&ctx->out_stream, 256);
        stream_init(&ctx->err_stream, 256);

        uv_pipe_init(&runner->loop, &ctx->out_pipe, 0);
        uv_pipe_init(&runner->loop, &ctx->err_pipe, 0);

        uv_stdio_container_t child_stdio[3];
        child_stdio[0].flags = UV_IGNORE;
        child_stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
        child_stdio[1].data.stream = (uv_stream_t *)&ctx->out_pipe;
        child_stdio[2].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
        child_stdio[2].data.stream = (uv_stream_t *)&ctx->err_pipe;

        uv_process_options_t options = {0};
        options.exit_cb = process_runner_on_exit;
        options.file = job->cmd->args[0];
        options.args = job->cmd->args;
        options.stdio_count = 3;
        options.stdio = child_stdio;

        int r = uv_spawn(&runner->loop, &ctx->process, &options);
        if (r) {
            // Mesmo com falha, a libuv exige fechar o handle do processo.
            ctx->spawn_error = r;
            job->exit_status = -1;
            runner->failed = true;
            uv_close((uv_handle_t *)&ctx->process, process_runner_on_handle_close);
            uv_close((uv_handle_t *)&ctx->out_pipe, process_runner_on_handle_close);
            uv_close((uv_handle_t *)&ctx->err_pipe, process_runner_on_handle_close);
            continue;
        }

        if (job->label && !job->capture) {
            printf("%s\n", job->label);
            fflush(stdout);
        }

        runner->active++;
        uv_read_start((uv_stream_t *)&ctx->out_pipe, process_runner_alloc_buffer, process_runner_read_out);
        uv_read_start((uv_stream_t *)&ctx->err_pipe, process_runner_alloc_buffer, process_runner_read_err);
    }
}

bool process_runner_run(process_job_t *jobs, word count, uint32 max_parallel) {
    RETURN_VAL_IF_FAIL(jobs || count == 0, false);

    process_runner_t runner = {
        .jobs = jobs,
        .count = count,
        .max_parallel = max_parallel ? max_parallel : platform_num_cores(),
    };

    int r = uv_loop_init(&runner.loop);
    if (r) {
        log_error("Falha ao iniciar o loop de eventos: %s", uv_strerror(r));
        return false;
    }

    process_runner_spawn_next(&runner);
    uv_run(&runner.loop, UV_RUN_DEFAULT);
    uv_loop_close(&runner.loop);

    return !runner.failed;
}
