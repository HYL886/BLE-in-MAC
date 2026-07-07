import subprocess

try:
    subprocess.check_call(["make", "clean"])
except subprocess.CalledProcessError as e:
    print(f"命令执行失败，退出状态码：{e.returncode}")


try:
    subprocess.check_call(["make", "all"])
except subprocess.CalledProcessError as e:
    print(f"命令执行失败，退出状态码：{e.returncode}")
