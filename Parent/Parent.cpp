#include <string>
#include <experimental/filesystem>

#include <libuv/uv.h>
#include <nfd/nfd.h>

namespace fs = std::experimental::filesystem;

uv_process_t process;

static void on_exit(uv_process_t* req, int64_t exit_status, int term_signal)
{
    fprintf(stderr, "Process exited with status %llu, signal %d\n", exit_status, term_signal);
    fflush(stderr);

    uv_close((uv_handle_t*)req, nullptr);
    process = { 0 };
}

static void run_process(const char* path)
{
    char *cargs[2];
    cargs[0] = (char*)malloc(strlen(path) + 1);
    strcpy(cargs[0], path);
    cargs[1] = NULL;

    // Pipe the stdout & stderr from the child process to this one
    uv_process_options_t options = { 0 };
    options.stdio_count = 3;
    uv_stdio_container_t child_stdio[3];
    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = UV_INHERIT_FD;
    child_stdio[1].data.fd = 1;
    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;
    options.stdio = child_stdio;

    options.exit_cb = on_exit;
    options.file = cargs[0];
    options.args = &cargs[0];

    // Spawn the child
    uv_process_t child_req = { 0 };
    int err = uv_spawn(uv_default_loop(), &child_req, &options);
    if (err)
    {
        fprintf(stderr, "Error spawning child process: %s\n", uv_strerror(err));
        return;
    }

    printf("Launched process with ID %d\n", child_req.pid);

    process = child_req;
}

static void open_file()
{
    nfdchar_t* outpath = nullptr;
    nfdresult_t result = NFD_OpenDialog("exe", fs::current_path().parent_path().string().c_str(), &outpath);

    if (result == NFD_OKAY)
    {
        run_process(outpath);

        free(outpath);
    }
    else if (result != NFD_CANCEL)
    {
        fprintf(stderr, "File system dialog error: %s\n", NFD_GetError());
    }
}

static void parse_commandline(uv_idle_t* handle)
{
    printf("- Press 'o' to open the child process--select \"Child.exe\".\n");
    printf("- Wait until you see the count down.\n");
    printf("- Then press 'c' to close the child process.\n\n");

    int ch;
    while ((ch = getchar()) != EOF)
    {
        switch (ch)
        {
        case 'o':
            open_file();
            break;
        case 'c':
            if (uv_process_get_pid(&process) > 0)
            {
                printf("Terminating process with id %d\n", uv_process_get_pid(&process));

                int err = uv_process_kill(&process, SIGTERM);
                if (err)
                {
                    fprintf(stderr, "Error killing process %d: %s\n", uv_process_get_pid(&process), uv_strerror(err));
                }
            }
            break;
        }
    }
}

int main()
{
    uv_idle_t idler = { 0 };
    uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, parse_commandline);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    uv_loop_close(uv_default_loop());

    return 0;
}