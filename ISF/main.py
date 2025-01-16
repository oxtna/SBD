import subprocess
import os
import glob
import random
import matplotlib.pyplot as plt


def cleanup() -> None:
    pattern = os.path.join(".", "chunk*")
    chunk_files = glob.glob(pattern)
    for chunk_file in chunk_files:
        try:
            os.remove(chunk_file)
        except:
            pass


def reset() -> None:
    with open("data.bin", "w"):
        pass
    with open("overflow.bin", "w"):
        pass
    with open("index.bin", "w"):
        pass

    cleanup()


def insert_statement(index: int, values: list[int]) -> str:
    return f"insert {index} {{ {", ".join([str(value) for value in values])} }};"


def update_statement(index: int, values: list[int]) -> str:
    return f"update {index} {{ {", ".join([str(value) for value in values])} }};"


def show_statement(index: int) -> str:
    return f"show {index};"


def list_statement() -> str:
    return "list;"


def sort_statement() -> str:
    return "sort;"


def status_statement() -> str:
    return "status;"


def linear_key_generation_test(last: int) -> None:
    reset()

    statements = (insert_statement(i, [0]) for i in range(1, last + 1))

    process = subprocess.Popen(
        "./ISF.exe",
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    stdout, _ = process.communicate(input="".join(statements))

    cleanup()

    stdout = stdout.lstrip("> ")
    stdout = stdout.split("\n")
    indices = []
    read_data = []
    write_data = []
    index = 0
    reads = 0
    writes = 0

    for line in stdout:
        if line.startswith("Reads:"):
            if index > 0:
                read_data.append(reads)
                indices.append(index)
            index += 1
            reads = int(line.split(" ")[-1])
        elif line.startswith("Reads during sorting:"):
            reads += int(line.split(" ")[-1])
        else:
            continue

    index = 0
    for line in stdout:
        if line.startswith("Writes:"):
            if index > 0:
                write_data.append(writes)
            index += 1
            writes = int(line.split(" ")[-1])
        elif line.startswith("Writes during sorting:"):
            writes += int(line.split(" ")[-1])
        else:
            continue

    _, ax = plt.subplots()
    ax.plot(indices, read_data, "r-", label="Odczyty")
    ax.plot(indices, write_data, "b-", label="Zapisy")
    ax.set_title("Liniowa generacja kluczy")
    ax.set_xlabel("Ilość kluczy")
    ax.set_ylabel("Ilość operacji")
    ax.legend()
    plt.show()


def random_key_generation_test(count: int) -> None:
    reset()

    statements = (
        insert_statement(i, [0]) for i in random.sample(range(0, count * 2), count)
    )

    process = subprocess.Popen(
        "./ISF.exe",
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    stdout, _ = process.communicate(input="".join(statements))

    cleanup()

    stdout = stdout.lstrip("> ")
    stdout = stdout.split("\n")
    indices = []
    read_data = []
    write_data = []
    index = 0
    reads = 0
    writes = 0

    for line in stdout:
        if line.startswith("Reads:"):
            if index > 0:
                read_data.append(reads)
                indices.append(index)
            index += 1
            reads = int(line.split(" ")[-1])
        elif line.startswith("Reads during sorting:"):
            reads += int(line.split(" ")[-1])
        else:
            continue

    index = 0
    for line in stdout:
        if line.startswith("Writes:"):
            if index > 0:
                write_data.append(writes)
            index += 1
            writes = int(line.split(" ")[-1])
        elif line.startswith("Writes during sorting:"):
            writes += int(line.split(" ")[-1])
        else:
            continue

    _, ax = plt.subplots()
    ax.plot(indices, read_data, "r-", label="Odczyty")
    ax.plot(indices, write_data, "b-", label="Zapisy")
    ax.set_title("Losowa generacja kluczy")
    ax.set_xlabel("Ilość kluczy")
    ax.set_ylabel("Ilość operacji")
    ax.legend()
    plt.show()


if __name__ == "__main__":
    linear_key_generation_test(200)
    random_key_generation_test(200)
    random_key_generation_test(200)
