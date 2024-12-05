##
## This file is part of the libsigrokdecode project.
##
## Copyright (C) 2015 Paul Evans <leonerd@leonerd.org.uk>
## Copyright (C) 2022 DreamSourceLab <support@dreamsourcelab.com>
## Copyright (C) 2024 Gilles Zunino <gzunino@akakani.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##

##
## 2022/07/05 DreamSourceLab : Support for different data output formats
## 2024/12/01 Gilles Zunino: Support chained MAX7219 / MAX7221. Display device index. Correct intensity value based on chip. Various bug fixes
##


import re
import sigrokdecode as srd


registers = {
    0x00: ['No-op'],
    0x09: ['Decode', lambda v: '0b{:08b}'.format(v)],
    0x0A: ['Intensity'],
    0x0B: ['Scan limit', lambda v: 1 + v],
    0x0C: ['Shutdown', lambda v: 'off' if v else 'on'],
    0x0F: ['Display test', lambda v: 'on' if v else 'off']
}

ann_reg = 0
ann_digit = 1
ann_invalid = 2
ann_device_index = 3

class Decoder(srd.Decoder):
    api_version = 3
    id = 'max7219_chain'
    name = 'MAX7219 Chain'
    longname = 'Maxim MAX7219/MAX7221 devices optionally arranged in cascade'
    desc = 'Maxim MAX72xx series 8-digit LED display driver.'
    license = 'gplv2+'
    inputs = ['spi']
    outputs = []
    tags = ['Display']
    options = (
        { 'id' : 'device_type' , 'desc' : 'Device type', 'default' : 'MAX7219', 'values' : ( 'MAX7219' , 'MAX7221' ) },
        { 'id' : 'chain_length' , 'desc' : 'Device count', 'default' : 1 }
    )
    annotations = (
        ('4', 'register', 'Registers written to the device'),
        ('11', 'digit', 'Digits displayed on the device'),
        ('0', 'invalid', 'Invalid register'),
        ('14', 'device', 'Device index')
    )
    annotation_rows = (
        ('commands', 'Commands', (ann_reg, ann_digit, ann_invalid)),
        ('device', 'Device', (ann_device_index,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.pos = 0
        self.cs_start = 0

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)
        self.device_index = self.options['chain_length']

    def putreg(self, ss, es, value):
        self.put(ss, es, self.out_ann, [ann_reg, ['%s' % value]])

    def putdigit(self, ss, es, digit, value):
        self.put(ss, es, self.out_ann, [ann_digit, ['Digit %d: {$}' % digit, '@%02X' % value]])
    
    def putinvalid(self, ss, es, value):
        self.put(ss, es, self.out_ann, [ann_invalid, ['%s' % value]])

    def putdevice_index(self, ss, es, index):
        self.put(ss, es, self.out_ann, [ann_device_index, ['Device %d' % index]])

    def decode_intensity(self, intensity):
        masked_intensity = intensity & 0x0f
        if self.options['device_type'] == 'MAX7219':
            return '%d/32' % (2 * masked_intensity + 1)
        else:
            masked_intensity += 1
            return '%d/16' % masked_intensity if masked_intensity != 16 else '15/16 max on'
        
    def decode(self, ss, es, data):
        ptype, mosi, _ = data

        if ptype == 'DATA':
            if not self.cs_asserted:
                return

            if self.pos == 0:
                self.addr = mosi
                self.addr_start = ss
                self.pos = 1
            elif self.pos == 1:
                sanitized_address = self.addr & 0x0f
                if sanitized_address >= 1 and sanitized_address <= 8:
                    self.putdigit(self.addr_start, es, sanitized_address, mosi)
                else:
                    if sanitized_address in registers:
                        if sanitized_address == 0x00:
                            annotation = registers[sanitized_address][0]
                        elif sanitized_address == 0x0A:
                            annotation = '%s: %s' % (registers[sanitized_address][0], self.decode_intensity(mosi))
                        else:
                            name, decoder = registers[sanitized_address]
                            annotation = '%s: %s' % (name, decoder(mosi))

                        self.putreg(self.addr_start, es, annotation)
                    else:
                        self.putinvalid(self.addr_start, es, 'INVALID REGISTER 0xX%01X' % sanitized_address)

                self.putdevice_index(self.addr_start, es, self.device_index)

                self.pos = 0
                self.device_index -= 1
                if self.device_index == 0:
                    self.device_index = self.options['chain_length']

        elif ptype == 'CS-CHANGE':
            self.cs_asserted = mosi
            if self.cs_asserted:
                self.pos = 0
                self.cs_start = ss
                self.device_index = self.options['chain_length']

