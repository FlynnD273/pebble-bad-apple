from PIL import Image, ImageChops, ImageFilter
from os import path
import glob
import pickle

INPUT_PATH = "./frames/"
OUTPUT_PATH = "./resources/raw/frames"

NUM_FRAMES = 740

files = glob.glob(path.join(INPUT_PATH, "*.png"))
files.sort()
files = files if NUM_FRAMES == -1 else files[:NUM_FRAMES]

frame_count = 0
runs = []
prev_img = Image.open(files[0]).convert("L").point(lambda p: 255 if p > 20 else 0, "1")  # type: ignore
for file in files:
    img = Image.open(file).convert("L").point(lambda p: 255 if p > 20 else 0, "1")  # type: ignore
    frame = ImageChops.logical_xor(prev_img, img)
    prev_img = img
    (width, height) = img.size
    is_counting_black = True
    run_count = 0
    for y in range(height):
        for x in range(width):
            val = frame.getpixel((x, y))
            is_black = val < 20  # type: ignore
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

with open("runs.pickle", "wb") as file:
    pickle.dump(runs, file)

# with open("runs.pickle", "rb") as file:
#     runs = pickle.load(file)

bits = []


def append(val: int, bit_length: int):
    for i in range(bit_length - 1, -1, -1):
        bits.append((val >> i) & 1)

    if val >> bit_length > 0:
        print("INVALID")
        exit(1)


for run in runs:
    if run < 0b11:
        append(0, 2)
        append(run, 2)
    elif run < 0xF:
        append(1, 2)
        append(run, 4)
    elif run < 0xFF:
        append(2, 2)
        append(run, 8)
    else:
        append(3, 2)
        append(run, 16)

with open(OUTPUT_PATH, "wb") as file:
    acc = 0
    shift = 0
    bits.reverse()
    while len(bits) >= 8:
        acc = 0
        for _ in range(8):
            bit = bits.pop()
            acc <<= 1
            acc |= bit
        file.write(bytes([acc]))

    end_length = len(bits)
    acc = 0
    for _ in range(end_length):
        acc <<= 1
        bit = bits.pop()
        acc |= bit
    acc <<= 8 - end_length
    file.write(bytes([acc]))

