import math

def srgb255_to_linear(i):
    c = i / 255.0
    if c <= 0.04045:
        c = c / 12.92
    else:
        c = ((c + 0.055) / 1.055) ** 2.4
    return c

def linear1_to_srgb255(i):
    c = i / 255.0
    if c <= 0.0031308:
        c = c * 12.92
    else:
        c = 1.055 * (c ** (1.0 / 2.4)) - 0.055
    c = c * 255.0 + 0.5
    c = max(0.0, min(255.0, c))
    return int(c)

COLS = 8

def print_float_lut(name, values):
    print(f"constexpr f32 {name}[256] = {{")
    for row in range(0, 256, COLS):
        chunk = values[row:row + COLS]
        formatted = ", ".join(f"{v:.9f}f" for v in chunk)
        comma = "," if row + COLS < 256 else ""
        print(f"    {formatted}{comma}")
    print("};\n")

def print_u32_lut(name, values):
    print(f"constexpr u32 {name}[256] = {{")
    for row in range(0, 256, COLS):
        chunk = values[row:row + COLS]
        formatted = ", ".join(f"{v:3d}" for v in chunk)
        comma = "," if row + COLS < 256 else ""
        print(f"    {formatted}{comma}")
    print("};\n")

srgb_to_linear = [srgb255_to_linear(i) for i in range(256)]
linear_to_srgb  = [linear1_to_srgb255(i) for i in range(256)]

print_float_lut("srgb255_to_linear_lut", srgb_to_linear)
print_u32_lut("linear1_to_srgb255_lut", linear_to_srgb)
