'''
Copyright (c) 2018 balika011

This program is free software; you can redistribute it and/or modify it
under the terms and conditions of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
'''

from keystone import *

CODE = b'''
	LDR	X1, =0x70019010
	MOV X0, #0x1
	STR W0, [X1]

loop:
	IC	IALLUIS
	DSB ISH
	B loop
	MOV X0, #0x0
	STR W0, [X1]
	LDR	X0, =0x4002B000
	BR	X0
'''
try:
	ks = Ks(KS_ARCH_ARM64, KS_MODE_LITTLE_ENDIAN)
	encoding, count = ks.asm(CODE, 0x0)
	print("%s = %s (number of statements: %u)" %(CODE, ', '.join([('0x%02x' % (x)) for x in encoding]), count))
except KsError as e:
	print("ERROR: %s" %e)