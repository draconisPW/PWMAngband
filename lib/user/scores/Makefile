MKPATH=../../../mk/
include $(MKPATH)buildsys.mk

PACKAGE = user/scores

install-extra:
	if [ "x$(SETEGID)" != "x" ]; then \
		if [ "x$(DRY)" = "x" ]; then \
			${MKDIR_P} ${DESTDIR}${vardatadir}${PACKAGE}; \
			touch ${DESTDIR}${vardatadir}${PACKAGE}/scores.raw; \
			chown -R root:${SETEGID} ${DESTDIR}${vardatadir}${PACKAGE}; \
			chmod -R 770 ${DESTDIR}${vardatadir}${PACKAGE}; \
		fi; \
	fi
