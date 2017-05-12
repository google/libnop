
M_OBJS := $(addprefix $(OUT_TARGET_OBJ)/,$(M_OBJS))
DEPS += $(M_OBJS:%o=%d)

ALL += $(OUT)/$(M_NAME).bin
ALL += $(OUT)/$(M_NAME).lst

$(OUT)/$(M_NAME).bin: _SRC := $(OUT)/$(M_NAME)
$(OUT)/$(M_NAME).bin: $(OUT)/$(M_NAME)
	@echo create $@
	$(QUIET)$(TARGET_OBJCOPY) --gap-fill=0xee -O binary $(_SRC) $@

$(OUT)/$(M_NAME).lst: _SRC := $(OUT)/$(M_NAME)
$(OUT)/$(M_NAME).lst: $(OUT)/$(M_NAME)
	@echo create $@
	$(QUIET)$(TARGET_OBJDUMP) -D $(_SRC) > $@

$(OUT)/$(M_NAME): _OBJS := $(M_OBJS)
$(OUT)/$(M_NAME): _LIBS := $(M_LIBS)
$(OUT)/$(M_NAME): _BASE := $(M_BASE)
$(OUT)/$(M_NAME): $(M_OBJS)
	@echo link $@
	$(QUIET)$(TARGET_LD) -Bstatic -T aboot.lds -Ttext $(_BASE) $(_OBJS) $(_LIBS) -o $@

M_OBJS :=
M_NAME :=
M_BASE :=
M_LIBS :=
