GLA11Y_OUTPUT = ui-a11y.err
GLA11Y_SUPPR  = ui-a11y.suppr
GLA11Y_FALSE  = ui-a11y.false

all-local: $(GLA11Y_OUTPUT)
$(GLA11Y_OUTPUT): $(ui_files)
	$(GLA11Y) -P $(srcdir)/ -f $(srcdir)/$(GLA11Y_FALSE) -s $(srcdir)/$(GLA11Y_SUPPR) -o $@ $(ui_files:%=$(srcdir)/%)
