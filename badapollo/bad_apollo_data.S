.section .rodata
.global _bad_apollo_data
.global _bad_apollo_data_end

_bad_apollo_data:
    .incbin "../bad_apollo_data.bin"
_bad_apollo_data_end: