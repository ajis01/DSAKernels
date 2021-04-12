RV_ROOT = $(SS_TOOLS)/riscv64-unknown-linux-gnu
SYSROOT = $(SS_TOOLS)/sysroot
MARCH   = rv64imac
ABI     = lp64
IFLAGS  = -I$(SS_TOOLS)/include                                              \
	      -I$(RV_ROOT)/include/c++/10.1.0/                                    \
	      -I$(SYSROOT)/usr/include                                    \
	      -I$(SS)/dsa-apps/common/include                                    \
	      -I$(RV_ROOT)/include/c++/10.1.0/riscv64-unknown-linux-gnu/$(MARCH)/$(ABI)/ \
	      -I$(RV_ROOT)/include/c++/10.1.0/riscv64-unknown-linux-gnu/ \
	      -I/root/ajis01/framework/dsa-examples/compile/vvision-kernels/

CLANG_FLAGS = -O2 -fno-inline -fno-vectorize -fno-slp-vectorize \
              -fno-unroll-loops -stream-specialize -emit-llvm -c -std=c++14   -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -L$(SYSROOT)/opencv/release/lib `pkg-config --cflags --libs opencv`

CLANG = clang++
OPT   = opt
LLC   = llc
GCC   = riscv64-unknown-linux-gnu-g++

# Compile pragma hinted high-level language to encoded metadata.
%.bc: %.cc
	$(CLANG) $< -o $@ $(CLANG_FLAGS) $(IFLAGS) --sysroot=$(SYSROOT) 

# Transform code regions to be accelerated to decoupled spatial IR.
dsa-%.bc: %.bc
	SBCONFIG=$(SS)/spatial.json                                \
	$(OPT) -load $(SS)/dsa-llvm-project/build/lib/DSAPass.so   \
		   -stream-specialize $< -o $@

# Apply general-purpose optimization to clean up the LLVM IR. 
O3-%.bc: %.bc
	$(OPT) -O3 < $< > $@

# Assembly code generation. Corresponding RISCV modeules should
# be enabled in the mattr flags.
%.s: %.bc
	$(LLC) -mattr=+m,+f,+a,+c $<

# For debugging purpose, dump bitcode to LLVM textformat.
%.ll: %.bc
	llvm-dis < $< > $@

# Use GCC tools to link the assembly code to executable binaries.
%.out: %.s
	$(GCC) $< -o $@ -march=$(MARCH) -mabi=$(ABI) -static -static-libgcc -static-libstdc++ -lc -lm 
	

clean:
	rm -f *.bc *.ll *.o *.out *.s *.dfg.h *.dfg
	rm -rf m5out sched viz verif stats

