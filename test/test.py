from rich import print
import pathlib
import subprocess

def main():
    print("Testing")
    test_folder = pathlib.Path(__file__).parent
    print(test_folder)

    # Iterate over the range 1 to 6 and run the command for each folder
    for i in range(1, 7):
        folder = f"./test/test{i}/"
        compile_command = f"{test_folder.parent}/build/gigly {folder} -o ./dump/exec"
        run_command = "./dump/exec"
        print(f"======== Compiling for {folder} ========")
        compile_result = subprocess.run(compile_command, shell=True)

        # Check the return code of the compile command
        if compile_result.returncode == 0:
            print(f"[green]Compilation successful for {folder}![/green]")
            print(f"======== Running executable for {folder} ========")
            run_result = subprocess.run(run_command, shell=True)
            if run_result.returncode == 0:
                print(f"[green]Execution successful for {folder}![/green]")
            else:
                print(f"[red]Execution failed for {folder} with return code:[/red]", run_result.returncode)
        else:
            print(f"[red]Compilation failed for {folder} with return code:[/red]", compile_result.returncode)

        print("=============================================")

if __name__ == "__main__":
    main()
