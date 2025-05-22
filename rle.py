from skimage import io, util, color
from os import path
import numpy as np
import sys
import glob

platform_type = sys.argv[1]

INPUT_PATH = "./frames/"
OUTPUT_PATH = "./resources/raw/frames" + ("~aplite" if platform_type == "aplite" else "")

MAX_FILE_SIZE = (128 if platform_type == "aplite" else 256) * 1000
NUM_FRAMES = -1

files = glob.glob(path.join(INPUT_PATH, "*.png"))
files.sort()
files = files if NUM_FRAMES == -1 else files[:NUM_FRAMES]

frame_count = 0
bits = []

split_threshold = (0.12 if platform_type == "aplite" else 0.015)


def split(img, x, y, w, h, threshold):
    x = int(x)
    y = int(y)
    w = int(w)
    h = int(h)
    mean = np.mean(img[y : y + h, x : x + w])
    if not (w < 4 or h < 4) and abs(mean - 0.5) < 0.5 - threshold:
        bits.append(1)
        new_threshold = (threshold + split_threshold) / 2
        split(img, x, y, w / 2, h / 2, new_threshold)
        split(img, x + w / 2, y, w / 2, h / 2, new_threshold)
        split(img, x, y + h / 2, w / 2, h / 2, new_threshold)
        split(img, x + w / 2, y + h / 2, w / 2, h / 2, new_threshold)
    else:
        bits.append(0)
        if mean < 0.5:
            bits.append(0)
        else:
            bits.append(1)

for file in files:
    img = util.img_as_float(color.rgb2gray(io.imread(file)))
    (height, width) = img.shape
    old_bits = bits.copy()
    split(img, 0, 0, width, height, 0.002)
    if len(bits) / 8 > MAX_FILE_SIZE:
        bits = old_bits
        break
    frame_count += 1
    print(f"\rFrame {frame_count}", end="     ")
print()


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

