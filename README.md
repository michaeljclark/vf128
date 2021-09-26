# vf128 variable length floating-point

_vf128_ is a variable length floating-point data format that succinctly
stores IEEE 754 floating-point values with up-to 120-bit mantissa and 24-bit
exponent, covering all floating-point types from `binary16` to `binary128`.

The _vf128_ variable length floating-point format provides:

- compact variable length storage of IEEE 754 floating-point values.
- mantissa encoding that supports quantization on 8-bit boundaries.
- succinct normalized values with unary exponent (-0.99999.. to 0.99999..).
- succinct power-of-two encoding with implicit mantissa.
- small floating point values inlined within the header:
  - -0.5 to 0.5 step 0.0625, -3.875 to 3.875 step 0.125,
    ±Zero, ±Inf, and ±NaN.

## format description

The _vf128_ format has a one byte header that contains a sign bit, a 2-bit
exponent length (0-3 bytes), a 4-bit mantissa length (0-15 bytes) and an
_extern bit_ that provides the ability to embed small floating point values
plus ±Zero, ±Inf, and ±NaN completely within the header byte. When the
_extern bit_ is clear, a floating-point value with 2-bit exponent and 4-bit
mantissa is contained within the header byte.

| extern | sign   | exponent (len)  | mantissa (len)                  |
|--------|--------|-----------------|---------------------------------|
| 1-bit  | 1-bit  | 2-bits          | 4-bits                          |

The header byte is optionally followed by little-endian exponent and mantissa,
with their lengths stored in the header exponent and mantissa length fields.
The presence of an external exponent, mantissa, or both, is indicated by the
extern bit being set and and at least one of the header exponent and mantissa
length fields being non-zero. The extern bit set with the header exponent and
mantissa length fields both set to zero is reserved for future use.

| exponent-payload (optional)      | mantissa-payload (optional)      |
|:---------------------------------|:---------------------------------|
| (0-3) x 8-bit packed exponent    | (0-15) x 8-bit packed mantissa   |

The exponent payload is an unbiased little-endian two's complement signed
integer. The exponent payload contains the equivalent of the IEEE 754
exponent less the bias, so unpacking the exponent requires one addition.

The mantissa payload is a little-endian integer with explicit leading
one, right-shifted so there are no trailing zeros. The point is to the
right of the leading one thus the exponent is the same as the normalized
IEEE 754 exponent.

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
are consistent with IEEE 754. The mantissa has an implied leading one bit,
except for the subnormal exponent value which has an implied leading zero.
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
- `extern = 0`
- `exponent = exponent[2-bits]`
- `mantissa = mantissa[4-bits]`

The inline values use the _float7_ format documented earlier.

#### zero

_Zero_ is encoded using exponent value of 0 and the mantissa set to zero:
- `extern = 0`
- `exponent = 0b00`
- `mantissa = 0b0000`

#### infinity

_Infinity_ is encoded using exponent value of 3 and the mantissa set to zero:
- `extern = 0`
- `exponent = 0b11`
- `mantissa = 0b0000`

#### not-a-number

_Not-a-Number_ is encoded using exponent value of 3 and the most significant
bit of the mantissa set:
- `extern = 0`
- `exponent = 0b11`
- `mantissa = 0b1000`

### normal floating point values

Values that cannot be inlined store their exponent and mantissa length in
the header.

The exponent is stored following the header as an unbiased little-endian
two's complement signed integer, encoded in as many bytes that are needed
to fit the exponent value and its leading sign bit.

The mantissa is stored following the exponent as a little-endian unsigned
integer with explicit leading one added, and trailing zeros removed so that
it can be stored in the minimum number of bytes.

- `extern = 1`
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

### powers-of-two

Powers of two are encoded with zero in the mantissa field and the exponent
length in the exponent field. The exponent bytes follow the header.

- `extern = 1`
- `exponent = length(exponent)`
- `mantissa = 0`
- `<exponent-payload>`

### normal values with unary exponent

Normal values in the range -0.99999.. to +0.99999.. with a binary exponent
from e-1 to e-8 inclusive are encoded with zero in the exponent field, and the
exponent is encoded as a unary prefix of trailing zeros in the mantissa field.

- `extern = 1`
- `exponent = 0`
- `mantissa = length(mantissa)`
- `<mantissa-payload>`

To decode, the leading zeros are counted to find a shift to left-justify
the fraction, and trailing zeros are counted to find the exponent. The
exponent is encoded as the negated trailing zeros count minus one.

- many float32 values between -0.99999.. to 0.99999 fit in 4 bytes or less
- most float64 values between -0.99999.. to 0.99999 fit in 8 bytes or less

The purpose of the normal value encoding rules are to save encoding space
for values fitting precision constraints.

- _ieee754-binary64_ values have a 53-bit mantissa thus most normal values will
  fit within the 7-byte (56-bit) quantized mantissa, due to there being 3-bits
  of space for the trailing zero unary prefix used to restore the exponent,
  thus most binary64 normal values require only 8 bytes of encoding space.
- _ieee754-binary32_ values have a 24-bit mantissa thus many normal values won't
  fit within the 3-byte (24-bit) quantized mantissa, due to there being 0-bits
  of space for the trailing zero unary prefix used to restore the exponent,
  thus many binary32 normal values require up to 5 bytes of encoding space.

### quantization and overflow

- Implementations may provide an interface to control encoding quantization
  by specifying a limit on the number of encoding bytes. Values can be made
  to fit within space equal to their IEEE 754 counterparts by sacrificing
  several ULP (units of precision in the last place).
- Implementations are required to truncate excess precision when reading a
  more precise value into a less precise floating-point type.
- Values with exponents falling outside the range of the floating-point type
  should be translated to ±Inf.

## build instructions

The reference implementation is written in C++11 and uses `cmake` thus
requires a modern C++ compiler installed:

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```
