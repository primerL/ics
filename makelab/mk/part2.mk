include $(HOME)/mk/config.mk

$(OUTPUT): main.o libB.a libA.a
	$(CXX) -o $@ $^

libA.a: notA.a.o A.a.o some.a.o
	$(AR) -r $@ $^

libB.a: B.b.o
	$(AR) -r $@ $^
