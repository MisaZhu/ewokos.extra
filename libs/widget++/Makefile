PROJS_ROOT_DIR=../..
include $(PROJS_ROOT_DIR)/make.inc

BUILD_DIR = $(EWOKOS_SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

# WidgetWebview is now a thin widget++ front-end over libewebview (built by
# ../../ewebview/ewebview into $(TARGET_DIR)/lib/libewebview.a, headers
# installed into $(TARGET_DIR)/include). All the web machinery (XContainer,
# JsCanvas, CookieJar, the JS/canvas glue, el_input) moved into ewebview.
LIB_WIDGET_WEBVIEW_OBJS = $(ARCH)/src/WidgetWebview/WidgetWebview.o
LIB_WIDGET_WEBVIEW = $(TARGET_DIR)/lib/libWidgetWebview.a

all: $(LIB_WIDGET_WEBVIEW)

$(LIB_WIDGET_WEBVIEW): $(LIB_WIDGET_WEBVIEW_OBJS)
	@mkdir -p $(TARGET_DIR)/lib
	@mkdir -p $(TARGET_DIR)/include
	$(AR) rcs $(LIB_WIDGET_WEBVIEW) $(LIB_WIDGET_WEBVIEW_OBJS)
	cp -r include/* $(TARGET_DIR)/include/


$(ARCH)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(ARCH)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(LIB_WIDGET_WEBVIEW_OBJS:.o=.d)

clean:
	rm -f $(LIB_WIDGET_WEBVIEW_OBJS)
	rm -f $(LIB_WIDGET_WEBVIEW_OBJS:.o=.d)
	rm -f $(LIB_WIDGET_WEBVIEW)
