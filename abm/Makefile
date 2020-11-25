#
# refind/Makefile
# Build control file for the rEFInd boot menu
#
# This program is licensed under the terms of the GNU GPL, version 3,
# or (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

SRCDIR = .

VPATH = $(SRCDIR)

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

TARGET	= abm.efi

ifeq ($(ARCH),ia32)
  LIBEG = build32
  TARGET = abm_ia32.efi
endif

ifeq ($(ARCH),x86_64)
  LIBEG = build64
  TARGET = abm_x64.efi
endif

ifeq ($(ARCH),aarch64)
  LIBEG = build
  TARGET = abm_aa64.efi
endif

LOCAL_GNUEFI_CFLAGS  = -I$(SRCDIR) -I$(SRCDIR)/../include \
LOCAL_LDFLAGS   = -L$(SRCDIR)/../EfiLib/
LOCAL_LIBS      = -lEfiLib

OBJS            = main.o

include $(SRCDIR)/../Make.common

all: $(TARGET)

$(SHLIB_TARGET): $(OBJS)
	$(LD) $(LOCAL_LDFLAGS) $(GNUEFI_LDFLAGS) $(SUBSYSTEM_LDFLAG) $(OBJS) \
	      -o $@ $(LOCAL_LIBS) $(GNUEFI_LIBS)

$(TARGET): $(SHLIB_TARGET)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
	           -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* \
		   -j .reloc $(FORMAT) $< $@
	chmod a-x $(TARGET)

clean:
	rm -f $(TARGET) *~ *.so $(OBJS) *.efi *.obj abm_*.txt \
		abm_*.dll *.lib

