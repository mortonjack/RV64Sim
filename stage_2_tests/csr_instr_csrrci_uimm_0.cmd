# Read/clear immediate a read-only CSR using 0 as uimm
# csr rs1 111 rd 1110011 CSRRCI

m 0 = F1307173  # csrrci x2, mimpid, 0

pc = 0
.

csr F13
x2

