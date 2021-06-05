# vf128 variable length floating-point

_vf128_ is a variable length floating-point data format that succinctly
stores IEEE 754 floating-point values with up-to 120-bit mantissa and 24-bit
exponent, covering all floating-point types from `binary16` to `binary128`.

The _vf128_ format has the following properties that provide for:

- compact variable length storage of floating-point values.
- exponent and mantissa encoded with leading zeros omitted.
- integers encoded with a mantissa and without an exponent.
- powers of two encoded with an exponent and without a mantissa.
- small floating point values that can be inlined within the header:
  - -0.5 to 0.5 step 0.0625, -3.875 to 3.875 step 0.125,
    ±Zero, ±Inf, and ±NaN.

## format description

The _vf128_ format is composed of a one byte header that stores the sign,
an exponent length (0-3 bytes), and a mantissa length (0-15 bytes). An
_inline bit_ provides the ability to encode small floating point values
plus ±Zero, ±Inf, and ±NaN completely within the header byte itself.
When the _inline bit_ is set, a floating-point value with 2-bit exponent
and 4-bit mantissa is contained within the header byte.

| inline | sign   | exponent (len)  | mantissa (len)                  |
|--------|--------|-----------------|---------------------------------|
| 1-bit  | 1-bit  | 2-bits          | 4-bits                          |

The header byte is optionally followed by little-endian exponent and
mantissa fields with lengths indicated by the header exponent and mantissa
fields. The presence of the _out-of-line_ exponent and mantissa following
the header is indicated by the inline bit being clear and header exponent
and mantissa fields being non-zero.

| exponent-data (optional)         | mantissa-data (optional)         |
|:---------------------------------|:---------------------------------|
| (0-3) x 8-bit packed exponent    | (0-15) x 8-bit packed mantissa   |

The exponent payload is an unbiased little-endian two's complement signed
integer. The exponent payload contains the equivalent of the IEEE 754
exponent less the bias, so unpacking the exponent requires one addition.

The mantissa payload is a little-endian unsigned integer with explicit
leading 1 added, and right-shifted so there are no trailing zeros. The
point is to the right of the leading one thus the exponent is the same
as the normalized IEEE 754 exponent.

![example vf128 encoding for -15.5](doc/vf128-example-1.svg)

_Figure 1: example vf128 encoding for -15.5_

### float7

The _vf128_ format contains an embedded format called _float7_ which
is the format used to inline values in the header.

| field    | size   | remarks                                         |
|:---------|:-------|:------------------------------------------------|
| sign     | 1-bit  |                                                 |
| exponent | 2-bits | _`0b00`=subnormal, `0b11`=infinity, bias=1_     |
| mantissa | 4-bits |                                                 |

The following values can inlined:
- -0.5 to 0.5 step 0.0625,
- -3.875 to 3.875 step 0.125.
- ±Zero, ±Inf, and ±NaN.

The 2-bit exponent has a bias of 1, a subnormal value of 0 and an infinity
value of 3. The rules for handling subnormals, infinity and not-a-number
are consistent with IEEE 754. The mantissa has an implied leading 1 bit,
except for the subnormal exponent value which has an implied leading 0 bit.
The rules are consistent with IEEE 754 graceful underflow. There are two
distinct exponent values and the minimum exponent is equal to _-(bias+1)_.

| exponent | description              | exponent   | example          |
|:---------|:-------------------------|:-----------|:-----------------|
| 0b00     | Subnormal, Zero          | 0          | `±0.1111 × 2⁰`   |
| 0b01     |                          | 0          | `±1.1111 × 2⁰`   |
| 0b10     |                          | 1          | `±1.1111 × 2ⁱ`   |
| 0b11     | Infinity, Not-a-Number   | N/A        |                  |

### inline floating-point values

Sufficiently small values can be encoded completely within the header byte:
- `inline = 1`
- `exponent = exponent[2-bits]`
- `mantissa = mantissa[4-bits]`

The inline values use the _float7_ format documented earlier.

#### infinity

_Infinity_ is encoded using exponent value of 3 and the mantissa set to zero:
- `inline = 1`
- `exponent = 0b11`
- `mantissa = 0b0000`

#### not-a-number

_Not-a-Number_ is encoded using exponent value of 3 and the most significant
bit of the mantissa set:
- `inline = 1`
- `exponent = 0b11`
- `mantissa = 0b1000`

### normal floating point values

Values that cannot be inlined store their exponent and mantissa length in
the header.

The exponent is stored following the header as an unbiased little-endian
two's complement signed integer, encoded in as many bytes that are needed
to fit the exponent value and its leading sign bit.

The mantissa is stored following the exponent as a little-endian unsigned
integer with explicit leading 1 added, and trailing zeros removed so that
it can be stored in the minimum number of bytes.

- `inline = 0`
- `exponent = length(exponent)`
- `mantissa = length(mantissa)`
- `<exponent-payload>`
- `<mantissa-payload>`

The mantissa is normalized to a form where the point is to the right of the
explicit leading one. This means the exponent simply needs the target bias
added when unpacking. The explicit leading one is used to left-justify
the fraction when unpacking it.

- To encode, the trailing zeros are counted to find shift to right-justify
  the fraction and add the explicit leading one.
- To decode, the leading zeros are counted to find shift to left-justify
  the fraction, and drop the explicit leading one.

### subnormals

IEEE 754 subnormals need to be normalized during encoding by subtracting the
leading zero count less one from the exponent to keep the exponent relative
to the explicit leading one. To decode, one simply detects values which have
an exponent less than the minimum exponent of the type that is being decoded
into, and shift the fraction accordingly.

### integers

Integers are encoded with zero in the exponent field, and the integer length
in the mantissa field. The integer bytes follow the header.

- `inline = 0`
- `exponent = 0`
- `mantissa = length(mantissa)`
- `<mantissa-payload>`

To decode, the leading zeros are counted to find a shift to left-justify
the integer, truncate the explicit leading one and set the exponent based
on the number of bits right of the leading one.

### powers-of-two

Powers of two are encoded with zero in the mantissa field and the exponent
length in the exponent field. The exponent bytes follow the header.

- `inline = 0`
- `exponent = length(exponent)`
- `mantissa = 0`
- `<exponent-payload>`

## build instructions

The reference implementation is written in C++11 and uses `cmake` thus
requires a modern C++ compiler installed:

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```
