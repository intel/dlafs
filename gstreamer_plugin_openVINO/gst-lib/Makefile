.PHONY: all clean

all:
	@$(MAKE) -C vpp
	@$(MAKE) -C common
	@$(MAKE) -C ocl
	@$(MAKE) -C algo
clean:
	@$(MAKE) clean -C vpp
	@$(MAKE) clean -C common
	@$(MAKE) clean -C ocl
	@$(MAKE) clean -C algo