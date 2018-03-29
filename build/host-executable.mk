MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
QUIET ?= @

M_OBJS := $(addprefix $(OUT_HOST_OBJ)/$(M_NAME)/,$(M_OBJS))
DEPS += $(M_OBJS:%o=%d)

ALL += $(OUT)/$(M_NAME)

$(OUT_HOST_OBJ)/$(M_NAME)/%.o: _CFLAGS := $(M_CFLAGS)
$(OUT_HOST_OBJ)/$(M_NAME)/%.o: _CXXFLAGS := $(M_CXXFLAGS)

$(OUT_HOST_OBJ)/$(M_NAME)/%.o: %.c
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(CC) $(HOST_CFLAGS) $(_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
$(OUT_HOST_OBJ)/$(M_NAME)/%.o: %.cpp
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(CXX) $(HOST_CFLAGS) $(HOST_CXXFLAGS) $(_CFLAGS) $(_CXXFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
$(OUT_HOST_OBJ)/$(M_NAME)/%.o: %.S
	@$(MKDIR)
	@echo assemble $<
	$(QUIET)$(CC) $(HOST_CFLAGS) $(_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)

$(OUT)/$(M_NAME): _OBJS := $(M_OBJS)
$(OUT)/$(M_NAME): _LDFLAGS := $(M_LDFLAGS)
$(OUT)/$(M_NAME): _CFLAGS := $(M_CFLAGS)
$(OUT)/$(M_NAME): _CXXFLAGS := $(M_CXXFLAGS)

$(OUT)/$(M_NAME): $(M_OBJS)
	@echo link $@
	$(QUIET)$(CXX) $(_CFLAGS) $(_CXXFLAGS) -o $@ $(_OBJS) $(HOST_LDFLAGS) $(HOST_CFLAGS) $(HOST_CXXFLAGS) $(_LDFLAGS)

M_OBJS :=
M_NAME :=
M_LDFLAGS :=
M_CFLAGS :=
M_CXXFLAGS :=
