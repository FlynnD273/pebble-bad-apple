import struct
from PIL import Image, ImageChops
from os import path
import glob

INPUT_PATH = "./frames/"
OUTPUT_PATH = "./resources/raw/frames"

NUM_FRAMES = 546

files = glob.glob(path.join(INPUT_PATH, "*.png"))
files.sort()
files = files if NUM_FRAMES == -1 else files[:NUM_FRAMES]

frame_count = 0
runs = []
prev_img = Image.open(files[0]).convert("1")
for file in files:
    img = Image.open(file).convert("1")
    frame = ImageChops.logical_xor(prev_img, img)
    prev_img = img
    (width, height) = img.size
    is_counting_black = True
    run_count = 0
    for y in range(height):
        for x in range(width):
            val = frame.getpixel((x, y))
            is_black = val < 20 # type: ignore
            if is_counting_black == is_black:
                run_count += 1
            else:
                is_counting_black = not is_counting_black
                runs.append(run_count)
                run_count = 1
    if run_count > 0:
        runs.append(run_count)
    frame_count += 1
    print(f"\rFrame {frame_count}", end="     ")
print()

with open(OUTPUT_PATH, "wb") as file:
    for run in runs:
        if run < 0xff:
            file.write(bytes([run]))
        else:
            file.write(bytes([0xff]))
            file.write(struct.pack(">H", run))
