
CC = g++
LL = g++
CFLAGS = -Wall -Wextra -Werror -g -I.
LDFLAGS = -pthread 

# Link with google test
GTEST = /usr/src/gtest
LDFLAGS += $(GTEST)/libgtest_main.a
LDFLAGS += $(GTEST)/libgtest.a

SRC = 
XSRC = test.cpp mock.cpp ../cli.cpp ../motor.cpp

APP = stepper
ODIR = obj
OBJDIR = $(ODIR)/c
XOBJDIR = $(ODIR)/cpp

OBJS = $(SRC:%.c=$(OBJDIR)/%.o)
DEPS = $(SRC:%.c=$(OBJDIR)/%.d)

XOBJS = $(XSRC:%.cpp=$(XOBJDIR)/%.o)
XDEPS = $(XSRC:%.cpp=$(XOBJDIR)/%.d)

MAKEDEPEND  = $(CC) -MM $(CPPFLAGS) -MT $(OBJDIR)/$*.o  -o $(OBJDIR)/$*.d $<
XMAKEDEPEND = $(CC) -MM $(CPPFLAGS) -MT $(XOBJDIR)/$*.o -o $(XOBJDIR)/$*.d $<

.SECONDARY:
	echo "x"

all: $(APP) $(DEPS)

clean:
	rm -r $(ODIR) -f $(APP)

test: $(APP)
	./$(APP)

$(APP): $(OBJS) $(XOBJS)
	$(LL) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.d : %.c Makefile
	@mkdir -p $(dir $(OBJDIR)/$<)
	$(MAKEDEPEND)

$(XOBJDIR)/%.d : %.cpp Makefile
	@mkdir -p $(dir $(XOBJDIR)/$<)
	$(XMAKEDEPEND)

$(OBJDIR)/%.o : %.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) -c -o $@ $<

$(XOBJDIR)/%.o : %.cpp $(XOBJDIR)/%.d
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(DEPS) 

# FIN
