
M_OBJS := $(addprefix $(OUT_HOST_OBJ)/,$(M_OBJS))
DEPS += $(M_OBJS:%o=%d)

ALL += $(OUT)/$(M_NAME)

$(OUT)/$(M_NAME): _OBJS := $(M_OBJS)
$(OUT)/$(M_NAME): $(M_OBJS)
	@echo link $@
	$(QUIET)g++ $(HOST_LDFLAGS) $(HOST_CFLAGS) $(HOST_CXXFLAGS) -o $@ $(_OBJS)

M_OBJS :=
M_NAME :=
