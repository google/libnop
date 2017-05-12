MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

QUIET ?= @

$(OUT_HOST_OBJ)/%.o: %.c
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(CC) $(HOST_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
$(OUT_HOST_OBJ)/%.o: %.cpp
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(CC) $(HOST_CFLAGS) $(HOST_CXXFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
$(OUT_HOST_OBJ)/%.o: %.S
	@$(MKDIR)
	@echo assemble $<
	$(QUIET)$(CC) $(HOST_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)

$(OUT_TARGET_OBJ)/%.o: %.c
	@$(MKDIR)
	@echo compile $<
	$(QUIET)$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
$(OUT_TARGET_OBJ)/%.o: %.S
	@$(MKDIR)
	@echo assemble $<
	$(QUIET)$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@ -MD -MT $@ -MF $(@:%o=%d)
