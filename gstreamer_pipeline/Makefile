.PHONY: all clean install

all:
	@make -C hddls_pipe_hal
	@cp hddls_pipe_hal/src/hddlspipes .
	@cp hddls_pipe_hal/tools/registeralgo .
clean:
	@make clean -C hddls_pipe_hal
install:
	@make install -C hddls_pipe_hal
	@echo ""
	@echo "Success to install hddlspipes"
	@echo ""
uninstall:
	@make uninstall -C hddls_pipe_hal
	@echo "Success to uninstall hddlspipes"
	@echo ""
