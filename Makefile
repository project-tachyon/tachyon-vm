SRC := $(CURDIR)
OUT := tachyon

CPP := clang++
CPPFLAGS := \
	-Werror \
	-Wextra \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-align \
	-Wwrite-strings \
	-Wredundant-decls \
	-Wformat \
	-Wformat-security \
	-I $(SRC)/Include/ \
	-O2

LDFLAGS := \
	-lstdc++ \
	-lSDL3 \
	-lc \
	-lzip

CPP_SRC := $(shell find -L * -type f -name "*.cpp")
CPP_OBJ := $(patsubst %.cpp, %.o, $(CPP_SRC))

$(OUT): $(CPP_OBJ)
	@echo "LD - $@"
	@$(CPP) $(LDFLAGS) $^ -o $@

%.o: %.c
	@$(CPP) $(CPPFLAGS) -c $< -o $@ $(CPPFLAGS)
	@echo "CPP - $<"

clean:
	@rm -fdr $(CPP_OBJ) $(OUT)
