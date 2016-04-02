artifacts: $(OBJDIR)/$(TARGET).elf $(OBJDIR)/$(TARGET).img tags

CFLAGS+=$(INCLUDES) -I. -MMD
CXXFLAGS+=$(INCLUDES) -I. -MMD

LFLAGS+=-Wl,-Map=$(OBJDIR)/$(TARGET).map

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo CXX $(<F)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif
	@echo $< >> $(OBJDIR)/sources.list

$(OBJDIR)/%.o: %.c | $(OBJDIR)
ifeq ($(VERBOSE),true)
	$(CC) $(CFLAGS) -c -o $@ $<
else
	@echo CC $(<F)
	@$(CC) $(CFLAGS) -c -o $@ $<
endif
	@echo $< >> $(OBJDIR)/sources.list

$(OBJDIR)/%.o: %.S | $(OBJDIR)
ifeq ($(VERBOSE),true)
	$(AS) $(AFLAGS) -c -o $@ $<
else
	@echo AS $(<F)
	@$(AS) $(AFLAGS) -c -o $@ $<
endif

$(OBJDIR)/%.i: %.c
	@$(CC) $(CFLAGS) -E -o $@ $<

$(OBJDIR)/%.i: %.cpp
	@$(CXX) $(CXXFLAGS) -E -o $@ $<

OBJECTS_IN_DIR:=$(addprefix $(OBJDIR)/,$(OBJECTS))

DEPS=$(OBJECTS_IN_DIR:.o=.d)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/$(TARGET).elf: $(OBJECTS_IN_DIR)
ifeq ($(VERBOSE),true)
	$(LD) $(LFLAGS) -o $@ $^ $(LDLIBS)
else
	@echo LD $@
	@$(LD) $(LFLAGS) -o $@ $^ $(LDLIBS)
endif
	@$(SIZE) $@

$(OBJDIR)/$(TARGET).img: $(OBJDIR)/$(TARGET).elf
	echo IMG $@
	@$(OBJDUMP) $< > $@

$(OBJDIR)/%.img: $(OBJDIR)/%.o
	echo IMG $@
	@$(OBJDUMP) -S $< > $@

vpath %.c $(C_VPATH)

vpath %.cpp $(CPP_VPATH)

vpath %.S $(S_VPATH)

clean:
	$(RM) -r $(OBJDIR) tags

tags: $(OBJDIR)/$(TARGET).elf
	@echo Make tags
	@cat $(OBJDIR)/*.d | tr " " "\n" | grep ".h$$" | sort | uniq > $(OBJDIR)/headers.list
	@cat $(OBJDIR)/sources.list $(OBJDIR)/headers.list > $(OBJDIR)/tagsinput.list
	@ctags -a -L $(OBJDIR)/tagsinput.list

run: $(OBJDIR)/$(TARGET).elf
	$(OPENOCD_BIN) -f $(OPENOCD_BOARD_SCRIPT) -l openocd.log -c "program $< verify reset exit"

.PHONY: artifacts clean run

-include $(DEPS)
