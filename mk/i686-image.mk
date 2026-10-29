image: all $(OUT)/image-tool$(HOSTEXE)
	mkdir -p "$(OUT)/root/bin" "$(OUT)/root/usr/bin" "$(OUT)/root/usr/libexec" "$(OUT)/root/etc" "$(OUT)/root/boot"
	cp "$(KERNEL)/$(KERNEL_OUT)/lume-i686.bin" "$(OUT)/root/boot/kernel.img"
	rm -f -- "$(OUT)/root/bin/sh"
	$(foreach app,ls dir cp touch busybox device video,cp "$(OUT)/$(app).elf" "$(OUT)/root/bin/$(app)";)
	cp "$(OUT)/libctest.elf" "$(OUT)/root/usr/libexec/libctest"
	cp "$(OUT)/worker.elf" "$(OUT)/root/usr/libexec/worker"
	rm -f -- "$(OUT)/root/usr/libexec/videotest"
	cp "$(OUT)/i686-video-test.elf" "$(OUT)/root/usr/libexec/vidtest"
	$(foreach app,$(ARM_APPLETS),MSYS2_ARG_CONV_EXCL='*' "$(OUT)/image-tool$(HOSTEXE)" alias /bin/busybox "$(OUT)/root/bin/$(app)";)
	"$(OUT)/image-tool$(HOSTEXE)" fat "$(OUT)/root" "$(OUT)/lume-root.fat"
