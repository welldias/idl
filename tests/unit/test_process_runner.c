#include "idl_test.h"
#include "process_runner.h"

#define MAX_JOBS 8

typedef struct {
    compiler_command_t cmds[MAX_JOBS];
    process_job_t jobs[MAX_JOBS];
    word count;
} jobs_t;

static void add_job(jobs_t *jobs, bool capture, int argc, char **argv) {
    word i = jobs->count++;
    compiler_command_init(&jobs->cmds[i], 4);
    compiler_commands_append(&jobs->cmds[i], argc, argv);
    jobs->jobs[i].cmd = &jobs->cmds[i];
    jobs->jobs[i].capture = capture;
}

static void clear_jobs(jobs_t *jobs) {
    for (word i = 0; i < jobs->count; i++) {
        compiler_command_clear(&jobs->cmds[i]);
        free(jobs->jobs[i].output);
    }
    memset(jobs, 0, sizeof(jobs_t));
}

static void test_all_succeed(void) {
    jobs_t jobs = {0};
    for (int i = 0; i < 4; i++)
        add_job(&jobs, false, 1, (char *[]){ IDL_TEST_FAKE_TOOL });

    CHECK(process_runner_run(jobs.jobs, jobs.count, 0));
    for (word i = 0; i < jobs.count; i++)
        CHECK_INT(jobs.jobs[i].exit_status, 0);

    clear_jobs(&jobs);
}

static void test_failure_is_reported(void) {
    jobs_t jobs = {0};
    add_job(&jobs, false, 1, (char *[]){ IDL_TEST_FAKE_TOOL });
    add_job(&jobs, false, 5, (char *[]){ IDL_TEST_FAKE_TOOL, "--stderr", "test error\n", "--exit", "2" });
    add_job(&jobs, false, 1, (char *[]){ IDL_TEST_FAKE_TOOL });

    CHECK(!process_runner_run(jobs.jobs, jobs.count, 0));
    CHECK_INT(jobs.jobs[0].exit_status, 0);
    CHECK_INT(jobs.jobs[1].exit_status, 2);
    CHECK_INT(jobs.jobs[2].exit_status, 0); // the others keep running

    clear_jobs(&jobs);
}

static void test_capture_output(void) {
    jobs_t jobs = {0};
    add_job(&jobs, true, 3, (char *[]){ IDL_TEST_FAKE_TOOL, "--stdout", "-I/x -DY" });
    add_job(&jobs, true, 1, (char *[]){ IDL_TEST_FAKE_TOOL });

    CHECK(process_runner_run(jobs.jobs, jobs.count, 0));
    CHECK_STR(jobs.jobs[0].output, "-I/x -DY");
    CHECK_STR(jobs.jobs[1].output, "");

    clear_jobs(&jobs);
}

static void test_spawn_failure(void) {
    jobs_t jobs = {0};
    add_job(&jobs, true, 1, (char *[]){ "idl-missing-program" });
    add_job(&jobs, false, 1, (char *[]){ IDL_TEST_FAKE_TOOL });

    CHECK(!process_runner_run(jobs.jobs, jobs.count, 0));
    CHECK_INT(jobs.jobs[0].exit_status, -1);
    CHECK_INT(jobs.jobs[1].exit_status, 0);

    clear_jobs(&jobs);
}

static void test_limited_parallelism(void) {
    jobs_t jobs = {0};
    for (int i = 0; i < MAX_JOBS; i++)
        add_job(&jobs, false, 1, (char *[]){ IDL_TEST_FAKE_TOOL });

    CHECK(process_runner_run(jobs.jobs, jobs.count, 1));
    for (word i = 0; i < jobs.count; i++)
        CHECK_INT(jobs.jobs[i].exit_status, 0);

    clear_jobs(&jobs);
}

static void test_no_jobs(void) {
    process_job_t none[1] = {0};
    CHECK(process_runner_run(none, 0, 0));
}

int main(void) {
    RUN_TEST(test_all_succeed);
    RUN_TEST(test_failure_is_reported);
    RUN_TEST(test_capture_output);
    RUN_TEST(test_spawn_failure);
    RUN_TEST(test_limited_parallelism);
    RUN_TEST(test_no_jobs);
    return idl_test_report();
}
