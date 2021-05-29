# vf128 variable length floating-point

vf128 variable length floating-point succinctly stores IEEE 754.1
floating-point values from `float16` to `float128`.

- compact variable length storage of floating-point values.
- exponent and mantissa stored without redundant zero bytes.
- integers stored with a mantissa and without an exponent.
- powers of two stored with an exponent and without a mantissa.
- small values inlined within the header byte:
  - -0.5 to 0.5 step 0.0625.
  - -3.875 to 3.875 step 0.125.
  - +/-Zero, +/-Inf, and +/-NaN.

## description

The vf128 format is composed of a one byte header that stores the sign,
an exponent length (0-3 bytes), and a mantissa length (0-15 bytes). An
inline bit aloows for small floating point values as Zero, +/-Inf, and
+/-NaN to be stored within the header byte itself.

| inline | sign   | exponent-len   | mantissa-len                   |
|--------|--------|----------------|--------------------------------|
| 1-bit  | 1-bit  | 2-bits         | 4-bits                         |

| exponent-data (optional)                                          |
|:------------------------------------------------------------------|
| (0-3) x 8-bit packets of the exponent                             |

| mantissa-data (optional)                                          |
|:------------------------------------------------------------------|
| (0-15) x 8-bit packets of the mantissa                            |

### inline floating-point values

Sufficiently small values can be encoded completely within the header byte:
- `inline = 1`
- `exponent = exponent[2-bits]`
- `mantissa = mantissa[4-bits]`

The following values can inlined:
- -0.5 to 0.5 step 0.0625,
- -3.875 to 3.875 step 0.125.
- +/-Zero, +/-Inf, and +/-NaN.

### out-of-line floating point values

Values that cannot be inlined store the exponent and mantissa length in
the header exponent and mantissa fields:

- `inline = 0`
- `exponent = length(exponent)`
- `mantissa = length(mantissa)`
- `<exponent-payload>`
- `<integer-payload>`

### integers

Integers are encoded with zero in the exponent field, and the integer length
in the mantissa field. The integer bytes follow the header.

- `inline = 0`
- `exponent = 0`
- `mantissa = length(mantissa)`
- `<integer-payload>`

### powers-of-two

Powers of two are encoded with zero in the mantissa field and the exponent
length in the exponent field. The exponent bytes follow the header.

- `inline = 0`
- `exponent = length(exponent)`
- `mantissa = 0`
- `<exponent-payload>`

## build instructions

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```
