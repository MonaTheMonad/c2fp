CPP = g++
CPPFLAGS = -I inc -MMD -MF $(basename $(notdir $@)).d
CXXFLAGS = -m64 -std=c++1z -Os -fno-inline -fno-stack-protector -fomit-frame-pointer
LIBFLAGS = -shared -flto

TEST = test
C2FP = libc2fp.so
LIBS = c++

$(C2FP): detail.o c2fp.o
	$(CPP) $(LIBFLAGS) -o $@ $^ $(foreach lib,$(LIBS),-l$(lib))

$(TEST): $(C2FP) test.o
	$(CPP) -o $@ $(filter %.o,$^) $(foreach lib,$(LIBS),-l$(lib)) -L. -lc2fp

%.o: src/%.cpp
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $(filter %.cpp,$^)

%.s: src/%.cpp
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -S -o $@ $(filter %.cpp,$^)

%.dmp: %.o
	objdump --disassemble -r $^ > $@

clean:
	rm -f *.dmp *.s *.o *.d

clear:
	rm -f $(TEST) $(C2FP)

sinclude *.d
