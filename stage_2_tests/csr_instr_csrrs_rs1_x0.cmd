# Read/set a read-only CSR using x0 as rs1
# csr rs1 010 rd 1110011 CSRRS

m 0 = F1302173  # csrrs x2, mimpid, x0

pc = 0
.

csr F13
x2

