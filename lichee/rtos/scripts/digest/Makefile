.PHONY:clean FORCE

srctree   := $(shell pwd)
target_digest_name = rv_digest
target_digest_src = $(wildcard $(srctree)/*.c)

# need to build a exc to pack dsp bin
$(target_digest_name): FORCE
	@gcc $(target_digest_src) -o $@
clean:
	@rm -rf $(target_digest_name)
