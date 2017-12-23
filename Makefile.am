
ACLOCAL_AMFLAGS = -I m4

init_ddir = $(sysconfdir)/init.d
defaultdir = $(sysconfdir)/default

bin_PROGRAMS = addr2name mapper get_reg_host get_sw_tx_host get_ustate_host
sbin_PROGRAMS = addr_serv

dist_bin_SCRIPTS = get_clock_port get_display get_host get_process get_reg_port get_server_port get_sw_timing_port get_sw_ustate_port get_title get_ustate_port get_workspace

dist_init_d_SCRIPTS = init.d/addr_serv
dist_default_DATA = default/addr_serv

addr_serv_SOURCES = server.c read_tab.c sgetline.c tpopen.c
include_HEADERS = server.h
addr2name_SOURCES = addr2name.c
mapper_SOURCES = mapper.c read_tab.c
get_reg_host_SOURCES = get_reg_host.c
get_sw_tx_host_SOURCES = get_sw_tx_host.c
get_ustate_host_SOURCES = get_ustate_host.c

install-exec-hook:
	-ln -s $(DESTDIR)$(bindir)/get_sw_tx_host $(DESTDIR)$(bindir)/get_sw_timing_host

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/get_sw_timing_host
