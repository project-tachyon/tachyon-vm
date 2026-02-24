SRC := $(CURDIR)
OUT := tachyon

CPP := clang++
CPPFLAGS := \
	-Wextra \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-align \
	-Wwrite-strings \
	-Wredundant-decls \
	-Wformat \
	-Wformat-security \
	-I $(SRC)/Include/ \
	-O3 \
	-msse

LDFLAGS := \
	-lstdc++ \
	-lSDL3 \
	-lc \
	-lzip

CPP_SRC := $(shell find -L * -type f -name "*.cpp")
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SRC))

$(OUT): $(CPP_OBJ)
	@echo "LD - $@"
	@$(CPP) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	@echo "CPP - $<"
	@$(CPP) $(CPPFLAGS) -c $< -o $@ $(CPPFLAGS)

clean:
	@rm -fdr $(CPP_OBJ) $(OUT)
