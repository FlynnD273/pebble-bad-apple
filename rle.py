import struct
from PIL import Image
from os import path
import glob

INPUT_PATH = "./frames/"
OUTPUT_PATH = "./resources/raw/frames"

NUM_FRAMES = 450

files = glob.glob(path.join(INPUT_PATH, "*.png"))
files.sort()
files = files if NUM_FRAMES == -1 else files[: NUM_FRAMES * 2 : 2]

frame = 0
runs = []
for file in files:
    img = Image.open(file)
    (width, height) = img.size
    is_counting_black = True
    run_count = 0
    for y in range(height):
        for x in range(width):
            val: float = img.getpixel((x, y))  # type: ignore
            is_black = val < 20
            if is_counting_black == is_black:
                run_count += 1
            else:
                is_counting_black = not is_counting_black
                runs.append(run_count)
                run_count = 1
    if run_count > 0:
        runs.append(run_count)
    frame += 1
    print(f"\rFrame {frame}", end="     ")
print()

with open(OUTPUT_PATH, "wb") as file:
    for run in runs:
        file.write(struct.pack(">H", run))

