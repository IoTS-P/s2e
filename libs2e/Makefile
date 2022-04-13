# Copyright (C) 2017, Cyberhaven
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

TARGETS := $(wildcard *-softmmu)

S2E_PREFIX?=$(CURDIR)/../opt
S2E_BUILD:=$(CURDIR)/../

all: $(TARGETS)
.PHONY: $(TARGETS)

$(TARGETS):
	$(MAKE) -C $@

clean:
	for f in $(TARGETS); do $(MAKE) -C $$f clean; done
install:
	install $(S2E_BUILD)/libs2e-release/arm-softmmu/libs2e.so $(S2E_PREFIX)/share/libs2e/libs2e-arm.so
	install $(S2E_BUILD)/libs2e-release/arm-s2e-softmmu/op_helper.bc.arm $(S2E_PREFIX)/share/libs2e/
	install $(S2E_BUILD)/libs2e-release/arm-s2e-softmmu/libs2e.so $(S2E_PREFIX)/share/libs2e/libs2e-arm-s2e.so
	install $(S2E_BUILD)/libs2e-release/arm-s2e_sp-softmmu/op_helper_sp.bc.arm $(S2E_PREFIX)/share/libs2e/
	install $(S2E_BUILD)/libs2e-release/arm-s2e_sp-softmmu/libs2e.so $(S2E_PREFIX)/share/libs2e/libs2e-arm-s2e_sp.so
