/*----------------------------------------------------------------
   Psikyo PS4 SH-2 Based Systems
   driver by David Haywood (+ Paul Priest)
------------------------------------------------------------------

This driver is for the dual-screen PS4 boards using an SH-2 processor

 Board PS4 (Custom Chip PS6807)
 ------------------------------
 Taisen Hot Gimmick (c)1997
 Taisen Hot Gimmick Kairakuten (c)1998
 Taisen Hot Gimmick 3 Digital Surfing (c)1999
 Taisen Hot Gimmick 4 Ever (c)2000
 Taisen Hot Gimmick Integral (c)2001

 Lode Runner - The Dig Fight (c)2000
 Quiz de Idol! Hot Debut (c)2000

 The PS4 board appears to be a cheaper board than PS3/5/5v2, with only simple sprites, no bgs,
 smaller palette etc, only 8bpp sprites too.
 Supports dual-screen though.

All the boards have

YMF278B-F (80 pin PQFP) & YAC513 (16 pin SOIC)
( YMF278B-F is OPL4 == OPL3 plus a sample playback engine. )

93C56 EEPROM
( 93c56 is a 93c46 with double the address space. )

To Do:

  Sprite List format not 100% understood.
  The sound rom banking is wrong, at least for the ROM tests (see hotgm4ev), all the roms
  are good, but it tests sound rom 0 twice due to the banking issues.

*-----------------------------------*
|         Tips and Tricks           |
*-----------------------------------*

Hold Button during booting to test roms (Checksum 16-bit) for:

Lode Runner - The Dig Fight:   PL1 Start (passes gfx, sample result:05A5, expects:0BB0 [both sets]) (banking?)
Quiz de Idol! Hot Debut:       PL1 Start (passes)

--- Lode Runner: The Dig Fight ---

5-0-8-2-0 Maintenance Mode

--- Quiz de Idol! Hot Debut ---

9-2-3-0-1 Maintenance Mode

NOTE: The version number (A/B) on Lode Runner: The Dig Fight is ONLY displayed when the game is set
      to Japanese.  The same is true for Space Bomber in psikyosh.c


Psikyo PS4V3 Hardware Overview
Psikyo, 1997-2001

PCB Layout
----------

PS4V3 MADE IN JAPAN
|--------------------------------------------------------------------|
| SND0.U10 SND1.U19               HA13118            HA13118         |
|                          JRC4741                                   |
| 8L.U112  8H.U113            YAC516    VOL             VOL        |-|
|                  74ACT153 74ACT153                               |
|                                                                  |-|
| 7L.U9    7H.U18  74HCT273 74HCT273 74ACT138 74HCT273               |
|                                             74HC244  74HC244      M|
|                                      74ACT244              CN8    A|
| 6L.U8    6H.U17   74ACT138  93LC56                                H|
|                                 JP4  74ACT244                     J|
|                       |---------|                                 O|
| 5L.U7    5H.U16       |         |    74ACT244              CN7    N|
|                       | PSIKYO  |             74HC14              G|
|                       | PS6807  |    74ACT244        TD62064       |
| 4L.U6    4H.U15       |         |                                |-|
|                       |---------|    74ACT244                    |
|                                                         TD62003  |-|
| 3L.U5    3H.U14       57.2727MHz     74ACT244                      |
|                                               74ACT138  TD62003    |
|                                               3771                J|
| 2L.U4    2H.U13   814260        74AC08 74ACT04 74HC245  74HC244   A|
|                                                                   M|
|                   814260                 |--------|               M|
| 1L.U3    1H.U12                          |        |               A|
|                                          |  SH-2  |                |
|                                          |        |     74HC244  |-|
| 0L.U2    0H.U11    1.U23                 |--------|              |
|                                                         74HC244  |-|
| PROG.U1            2.U22                     74ACT139              |
|--------------------------------------------------------------------|
(All IC's listed)
Notes:
SH-2    - Hitachi HD6417604F28 SH-2 CPU, clock input 28.63635 [57.2727/2] (QFP144)
YMF278  - Yamaha YMF278B OPL4 sound chip, clock input 28.63635MHz [57.2727/2] (QFP80)
YAC516  - Yamaha YAC516 Delta Sigma Modulation D/A Converter with 8 Times Over-sampling
          Filter. Clock input 14.318175MHz [57.2727/4]
814260  - Fujitsu 814260-70 256k x16 (4MBit) DRAM (SOJ40)
3771    - Fujitsu MB3771 Watchdog Reset IC (SOIC8)
93LC56  - 256x8(2k) Serial CMOS EEPROM (SOIC8)
JRC4741 - Japan Radio Co. JRC4741 Quad OP AMP (SOIC14)
HA13118 - 18W Bridge Tied Load (BTL) Mono Audio Amplifier Module (in 15-pin SP-15TA package)
JP4     - Hardwired jumper bank (x4) for region selection.
          All jumpers shorted = Japan region (default)
CN7/CN8 - Extra connector for 2nd screen output and additional player inputs. JAMMA/MAHJONG
          connectors not used together. Either one or the other is used depending on the game.
          Game will operate fine with 1 screen output. CN7/CN8 connection is optional.
TD62064 - Toshiba TD62064 4 Channel High-Current Darlington Sink Driver (SOP18)
TD62003 - Toshiba TD62003 7 Channel High-Current Darlington Sink Driver (SOP16)
VSync   - 60Hz
HSync   - 15.68kHz

ROMs -
       1.U23 - Main Program, ST 27C4002 EPROM (DIP40)
       2.U22 /

       The remaining ROMs are surface mounted TSOP48 Type II MASKROMs,
       either OKI MSM27C3252 (32MBit) or OKI MSM27C1652 (16MBit).
       These MASKROMs are non-standard are require a custom adapter
       to read them. Not all positions are populated for each game. See
       the source below for specifics.

----------------------------------------------------------------*/

#include "driver.h"

#include "cpu/sh2/sh2.h"
#include "machine/eeprom.h"
#include "sound/ymf278b.h"
#include "rendlay.h"

#define ROMTEST 1 /* Does necessary stuff to perform rom test, uses RAM as it doesn't dispose of GFX after decoding */

UINT32 *psikyo4_vidregs;
static UINT32 *ps4_ram, *ps4_io_select;
static UINT32 *bgpen_1, *bgpen_2;

#define MASTER_CLOCK 57272700	// main oscillator frequency

/* defined in video/psikyo4.c */
VIDEO_START( psikyo4 );
VIDEO_UPDATE( psikyo4 );

static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{STEP8(0,1)},
	{STEP16(0,8)},
	{STEP16(0,16*8)},
	16*16*8
};

static GFXDECODE_START( ps4 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, layout_16x16x8, 0x000, 0x80 ) // 8bpp tiles
GFXDECODE_END

static const eeprom_interface eeprom_interface_93C56 =
{
	8,		// address bits 8
	8,		// data bits    8
	"*110x",	// read         110x aaaaaaaa
	"*101x",	// write        101x aaaaaaaa dddddddd
	"*111x",	// erase        111x aaaaaaaa
	"*10000xxxxxxx",// lock         100x 00xxxx
	"*10011xxxxxxx",// unlock       100x 11xxxx
//  "*10001xxxx",   // write all    1 00 01xxxx dddddddddddddddd
//  "*10010xxxx"    // erase all    1 00 10xxxx
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		eeprom_save(file);
	}
	else
	{
		eeprom_init(&eeprom_interface_93C56);
		if (file)
		{
			eeprom_load(file);
		}
		else	// these games want the eeprom all zeros by default
		{
			UINT32 length, size;
			UINT8 *dat;

			dat = eeprom_get_data_pointer(&length, &size);
			memset(dat, 0, length * size);
		}
	}
}

static WRITE32_HANDLER( ps4_eeprom_w )
{
	if (ACCESSING_BITS_16_31)
	{
		eeprom_write_bit((data & 0x00200000) ? 1 : 0);
		eeprom_set_cs_line((data & 0x00800000) ? CLEAR_LINE : ASSERT_LINE);
		eeprom_set_clock_line((data & 0x00400000) ? ASSERT_LINE : CLEAR_LINE);

		return;
	}

	logerror("Unk EEPROM write %x mask %x\n", data, mem_mask);
}

static READ32_HANDLER( ps4_eeprom_r )
{
	if (ACCESSING_BITS_16_31)
	{
		return ((eeprom_read_bit() << 20)); /* EEPROM */
	}

//  logerror("Unk EEPROM read mask %x\n", mem_mask);

	return input_port_read(machine, "JP4")<<16;
}

static INTERRUPT_GEN(psikyosh_interrupt)
{
	cpunum_set_input_line(machine, 0, 4, HOLD_LINE);
}

static READ32_HANDLER(hotgmck_io32_r) /* used by hotgmck/hgkairak */
{
	int ret = 0xff;
	int sel = (ps4_io_select[0] & 0x0000ff00) >> 8;

	if (sel & 1) ret &= input_port_read(machine, offset ? "KEY4" : "KEY0" );
	if (sel & 2) ret &= input_port_read(machine, offset ? "KEY5" : "KEY1" );
	if (sel & 4) ret &= input_port_read(machine, offset ? "KEY6" : "KEY2" );
	if (sel & 8) ret &= input_port_read(machine, offset ? "KEY7" : "KEY3" );

	return ret<<24 | input_port_read(machine, "SYSTEM");
}

static READ32_HANDLER(ps4_io32_r) /* used by loderndf/hotdebut */
{
	return ((input_port_read(machine, offset ? "P3" : "P1" ) << 24) | (input_port_read(machine, offset ? "P4" : "P2" ) << 16) | (input_port_read(machine, offset ? "UNUSED1" : "UNUSED0" ) << 8) | (input_port_read(machine, offset ? "UNUSED2" : "SYSTEM" ) << 0));
}

static WRITE32_HANDLER( ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(machine,offset,MAKE_RGB(r,g,b));
	palette_set_color(machine,offset+0x800,MAKE_RGB(r,g,b)); // For screen 2
}

static WRITE32_HANDLER( ps4_bgpen_1_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_1[0]);

	b = ((bgpen_1[0] & 0x0000ff00) >>8);
	g = ((bgpen_1[0] & 0x00ff0000) >>16);
	r = ((bgpen_1[0] & 0xff000000) >>24);

	palette_set_color(machine,0x1000,MAKE_RGB(r,g,b)); // Clear colour for screen 1
}

static WRITE32_HANDLER( ps4_bgpen_2_dword_w )
{
	int r,g,b;
	COMBINE_DATA(&bgpen_2[0]);

	b = ((bgpen_2[0] & 0x0000ff00) >>8);
	g = ((bgpen_2[0] & 0x00ff0000) >>16);
	r = ((bgpen_2[0] & 0xff000000) >>24);

	palette_set_color(machine,0x1001,MAKE_RGB(r,g,b)); // Clear colour for screen 2
}

static WRITE32_HANDLER( ps4_screen1_brt_w )
{
	if(ACCESSING_BITS_0_7) {
		/* Need seperate brightness for both screens if displaying together */
		double brt1 = data & 0xff;
		static double oldbrt1;

		if (brt1>0x7f) brt1 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt1 = (0x7f - brt1) / 127.0;
		if (oldbrt1 != brt1)
		{
			int i;

			for (i = 0; i < 0x800; i++)
				palette_set_brightness(machine,i,brt1);

			oldbrt1 = brt1;
		}
	} else {
		/* I believe this to be seperate rgb brightness due to strings in hotdebut, unused in 4 dumped games */
		if((data & mem_mask) != 0)
			logerror("Unk Scr 1 rgb? brt write %08x mask %08x\n", data, mem_mask);
	}
}

static WRITE32_HANDLER( ps4_screen2_brt_w )
{
	if(ACCESSING_BITS_0_7) {
		/* Need seperate brightness for both screens if displaying together */
		double brt2 = data & 0xff;
		static double oldbrt2;

		if (brt2>0x7f) brt2 = 0x7f; /* I reckon values must be clamped to 0x7f */

		brt2 = (0x7f - brt2) / 127.0;

		if (oldbrt2 != brt2)
		{
			int i;

			for (i = 0x800; i < 0x1000; i++)
				palette_set_brightness(machine,i,brt2);

			oldbrt2 = brt2;
		}
	} else {
		/* I believe this to be seperate rgb brightness due to strings in hotdebut, unused in 4 dumped games */
		if((data & mem_mask) != 0)
			logerror("Unk Scr 2 rgb? brt write %08x mask %08x\n", data, mem_mask);
	}
}

static WRITE32_HANDLER( ps4_vidregs_w )
{
	COMBINE_DATA(&psikyo4_vidregs[offset]);

#if ROMTEST
	if(offset==2) /* Configure bank for gfx test */
	{
		if (ACCESSING_BITS_0_15)	// Bank
		{
			UINT8 *ROM = memory_region(machine, REGION_GFX1);
			memory_set_bankptr(2,&ROM[0x2000 * (psikyo4_vidregs[offset]&0x1fff)]); /* Bank comes from vidregs */
		}
	}
#endif
}

#if ROMTEST
static UINT32 sample_offs = 0;

static READ32_HANDLER( ps4_sample_r ) /* Send sample data for test */
{
	UINT8 *ROM = memory_region(machine, REGION_SOUND1);
	return ROM[sample_offs++]<<16;
}
#endif

static READ32_HANDLER( psh_ymf_fm_r )
{
	return YMF278B_status_port_0_r(machine, 0)<<24; /* Also, bit 0 being high indicates not ready to send sample data for test */
}

static WRITE32_HANDLER( psh_ymf_fm_w )
{
	if (ACCESSING_BITS_24_31)	// FM bank 1 address (OPL2/OPL3 compatible)
	{
		YMF278B_control_port_0_A_w(machine, 0, data>>24);
	}

	if (ACCESSING_BITS_16_23)	// FM bank 1 data
	{
		YMF278B_data_port_0_A_w(machine, 0, data>>16);
	}

	if (ACCESSING_BITS_8_15)	// FM bank 2 address (OPL3/YMF 262 extended)
	{
		YMF278B_control_port_0_B_w(machine, 0, data>>8);
	}

	if (ACCESSING_BITS_0_7)	// FM bank 2 data
	{
		YMF278B_data_port_0_B_w(machine, 0, data);
	}
}

static WRITE32_HANDLER( psh_ymf_pcm_w )
{
	if (ACCESSING_BITS_24_31)	// PCM address (OPL4/YMF 278B extended)
	{
		YMF278B_control_port_0_C_w(machine, 0, data>>24);

#if ROMTEST
		if (data>>24 == 0x06)	// Reset Sample reading (They always write this code immediately before reading data)
		{
			sample_offs = 0;
		}
#endif
	}

	if (ACCESSING_BITS_16_23)	// PCM data
	{
		YMF278B_data_port_0_C_w(machine, 0, data>>16);
	}
}

#define PCM_BANK_NO(n)	((ps4_io_select[0] >> (n * 4 + 24)) & 0x07)

static void set_hotgmck_pcm_bank(running_machine *machine, int n)
{
	UINT8 *ymf_pcmbank = memory_region(machine, REGION_SOUND1) + 0x200000;
	UINT8 *pcm_rom = memory_region(machine, REGION_SOUND2);

	memcpy(ymf_pcmbank + n * 0x100000, pcm_rom + PCM_BANK_NO(n) * 0x100000, 0x100000);
}

static WRITE32_HANDLER( hotgmck_pcm_bank_w )
{
	int old_bank0 = PCM_BANK_NO(0);
	int old_bank1 = PCM_BANK_NO(1);
	int new_bank0, new_bank1;

	COMBINE_DATA(&ps4_io_select[0]);

	new_bank0 = PCM_BANK_NO(0);
	new_bank1 = PCM_BANK_NO(1);

	if (old_bank0 != new_bank0)
		set_hotgmck_pcm_bank(machine, 0);

	if (old_bank1 != new_bank1)
		set_hotgmck_pcm_bank(machine, 1);
}

static ADDRESS_MAP_START( ps4_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_READ(SMH_ROM)	// program ROM (1 meg)
	AM_RANGE(0x02000000, 0x021fffff) AM_READ(SMH_BANK1) // data ROM
	AM_RANGE(0x03000000, 0x030037ff) AM_READ(SMH_RAM)
	AM_RANGE(0x03003fe0, 0x03003fe3) AM_READ(ps4_eeprom_r)
	AM_RANGE(0x03003fe4, 0x03003fe7) AM_READ(SMH_NOP) // also writes to this address - might be vblank?
//  AM_RANGE(0x03003fe8, 0x03003fef) AM_READ(SMH_RAM) // vid regs?
	AM_RANGE(0x03004000, 0x03005fff) AM_READ(SMH_RAM)
	AM_RANGE(0x05000000, 0x05000003) AM_READ(psh_ymf_fm_r) // read YMF status
	AM_RANGE(0x05800000, 0x05800007) AM_READ(ps4_io32_r) // Screen 1+2's Controls
	AM_RANGE(0x06000000, 0x060fffff) AM_READ(SMH_RAM)	// main RAM (1 meg)

#if ROMTEST
	AM_RANGE(0x05000004, 0x05000007) AM_READ(ps4_sample_r) // data for rom tests (Used to verify Sample rom)
	AM_RANGE(0x03006000, 0x03007fff) AM_READ(SMH_BANK2) // data for rom tests (gfx), data is controlled by vidreg
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( ps4_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_WRITE(SMH_ROM)	// program ROM (1 meg)
	AM_RANGE(0x03000000, 0x030037ff) AM_WRITE(SMH_RAM) AM_BASE(&spriteram32) AM_SIZE(&spriteram_size)
	AM_RANGE(0x03003fe0, 0x03003fe3) AM_WRITE(ps4_eeprom_w)
//  AM_RANGE(0x03003fe4, 0x03003fe7) AM_WRITE(SMH_NOP) // might be vblank?
	AM_RANGE(0x03003fe4, 0x03003fef) AM_WRITE(ps4_vidregs_w) AM_BASE(&psikyo4_vidregs) // vid regs?
	AM_RANGE(0x03003ff0, 0x03003ff3) AM_WRITE(ps4_screen1_brt_w) // screen 1 brightness
	AM_RANGE(0x03003ff4, 0x03003ff7) AM_WRITE(ps4_bgpen_1_dword_w) AM_BASE(&bgpen_1) // screen 1 clear colour
	AM_RANGE(0x03003ff8, 0x03003ffb) AM_WRITE(ps4_screen2_brt_w) // screen 2 brightness
	AM_RANGE(0x03003ffc, 0x03003fff) AM_WRITE(ps4_bgpen_2_dword_w) AM_BASE(&bgpen_2) // screen 2 clear colour
	AM_RANGE(0x03004000, 0x03005fff) AM_WRITE(ps4_paletteram32_RRRRRRRRGGGGGGGGBBBBBBBBxxxxxxxx_dword_w) AM_BASE(&paletteram32) // palette
	AM_RANGE(0x05000000, 0x05000003) AM_WRITE(psh_ymf_fm_w) // first 2 OPL4 register banks
	AM_RANGE(0x05000004, 0x05000007) AM_WRITE(psh_ymf_pcm_w) // third OPL4 register bank
	AM_RANGE(0x05800008, 0x0580000b) AM_WRITE(SMH_RAM) AM_BASE(&ps4_io_select) // Used by Mahjong games to choose input (also maps normal loderndf inputs to offsets)
	AM_RANGE(0x06000000, 0x060fffff) AM_WRITE(SMH_RAM) AM_BASE(&ps4_ram)	// work RAM
ADDRESS_MAP_END

static void irqhandler(running_machine *machine, int linestate)
{
	if (linestate)
		cpunum_set_input_line(machine, 0, 12, ASSERT_LINE);
	else
		cpunum_set_input_line(machine, 0, 12, CLEAR_LINE);
}

static const struct YMF278B_interface ymf278b_interface =
{
	REGION_SOUND1,
	irqhandler
};

static MACHINE_DRIVER_START( ps4big )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, MASTER_CLOCK/2)
	MDRV_CPU_PROGRAM_MAP(ps4_readmem,ps4_writemem)
	MDRV_CPU_VBLANK_INT("left", psikyosh_interrupt)

	MDRV_NVRAM_HANDLER(93C56)

	/* video hardware */
	MDRV_GFXDECODE(ps4)
	MDRV_PALETTE_LENGTH((0x2000/4)*2 + 2) /* 0x2000/4 for each screen. 1 for each screen clear colour */
	MDRV_DEFAULT_LAYOUT(layout_dualhsxs)

	MDRV_SCREEN_ADD("left", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)

	MDRV_SCREEN_ADD("right", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)

	MDRV_VIDEO_START(psikyo4)
	MDRV_VIDEO_UPDATE(psikyo4)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD("ymf", YMF278B, MASTER_CLOCK/2)
	MDRV_SOUND_CONFIG(ymf278b_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ps4small )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(ps4big)

	MDRV_SCREEN_MODIFY("left")
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)

	MDRV_SCREEN_MODIFY("right")
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
MACHINE_DRIVER_END



#define UNUSED_PORT \
	/* not read? */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

static INPUT_PORTS_START( hotgmck )
	PORT_START_TAG("KEY0")	/* fake player 1 controls 1st bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY1")	/* fake player 1 controls 2nd bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY2")	/* fake player 1 controls 3rd bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY3")	/* fake player 1 controls 4th bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY4")	/* fake player 2 controls 1st bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY5")	/* fake player 2 controls 2nd bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY6")	/* fake player 2 controls 3rd bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("KEY7")	/* fake player 2 controls 4th bank */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("SYSTEM")	/* system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 2
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_SERVICE_NO_TOGGLE( 0x20, IP_ACTIVE_LOW)
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Unknown effects */
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START_TAG("JP4")/* jumper pads 'JP4' on the PCB */
	UNUSED_PORT
INPUT_PORTS_END

static INPUT_PORTS_START( loderndf )
	PORT_START_TAG("P1")	/* player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) // Can be used as Retry button
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("P2")	/* player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) // Can be used as Retry button
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("UNUSED0")
	UNUSED_PORT /* unused? */

	PORT_START_TAG("SYSTEM")	/* system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 1 - 2nd slot
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) // Screen 2
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) // Screen 2 - 2nd slot
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_SERVICE_NO_TOGGLE( 0x20, IP_ACTIVE_LOW)
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Must be high for rom test, unknown other side-effects */
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START_TAG("P3")	/* player 1 controls on second screen */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3) // Can be used as Retry button
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START_TAG("P4")	/* player 2 controls on second screen */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4) // Can be used as Retry button
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START_TAG("UNUSED1")
	UNUSED_PORT

	PORT_START_TAG("UNUSED2")
	UNUSED_PORT

	PORT_START_TAG("JP4")/* jumper pads 'JP4' on the PCB */
//  1-ON,2-ON,3-ON,4-ON  --> Japanese
//  1-ON,2-ON,3-ON,4-OFF --> English
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Region ) )
	PORT_DIPSETTING(	0x00, "Japan (Shows Version Number)" )
	PORT_DIPSETTING(	0x01, "World (Does Not Show Version Number)" )
INPUT_PORTS_END

/* unused inputs also act as duplicate buttons */
static INPUT_PORTS_START( hotdebut )
	PORT_START_TAG("P1")	/* player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("P2")	/* player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("UNUSED0")
	UNUSED_PORT

	PORT_START_TAG("SYSTEM")	/* system inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1    ) // Screen 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2    ) // Screen 1 - 2nd slot
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3    ) // Screen 2
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN4    ) // Screen 2 - 2nd slot
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) // Screen 1
	PORT_SERVICE_NO_TOGGLE(0x20, IP_ACTIVE_LOW)
#if ROMTEST
	PORT_DIPNAME( 0x40, 0x40, "Debug" ) /* Must be high for rom test, unknown other side-effects */
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SERVICE2 ) // Screen 2

	PORT_START_TAG("P3")	/* player 1 controls on second screen */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START_TAG("P4")	/* player 2 controls on second screen */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START_TAG("UNUSED1")
	UNUSED_PORT

	PORT_START_TAG("UNUSED2")
	UNUSED_PORT

	PORT_START_TAG("JP4")/* jumper pads 'JP4' on the PCB */
	UNUSED_PORT
INPUT_PORTS_END

#if ROMTEST
#define ROMTEST_GFX 0
#else
#define ROMTEST_GFX ROMREGION_DISPOSE
#endif

ROM_START( hotgmck )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2-u23.bin", 0x000002, 0x080000, CRC(23ed4aa5) SHA1(bb4f57a6adffc6336fc572a4ff1f5dfc284ee4fb) )
	ROM_LOAD32_WORD_SWAP( "1-u22.bin", 0x000000, 0x080000, CRC(5db3649f) SHA1(6ea7bd18bcf6224ed9b0480bb59c684f13b71d8a) )
	ROM_LOAD16_WORD_SWAP( "prog.bin",  0x100000, 0x200000, CRC(500f6b1b) SHA1(4ce454e44da08e351a81ca4b670ff3e080dcb330) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.bin", 0x0000000, 0x400000, CRC(91f9ba60) SHA1(968de0cd275784cf082df172b7f205861a8fbae4) )
	ROM_LOAD32_WORD( "0h.bin", 0x0000002, 0x400000, CRC(bfa800b7) SHA1(8e4a026f20b7ba035eb78713d64bfcb611c90640) )
	ROM_LOAD32_WORD( "1l.bin", 0x0800000, 0x400000, CRC(4b670809) SHA1(c0fe45d7618653b089ec293e7661da682b77534d) )
	ROM_LOAD32_WORD( "1h.bin", 0x0800002, 0x400000, CRC(ab513a4d) SHA1(4d589545765f55aaddb05f484fbf6248af3e237b) )
	ROM_LOAD32_WORD( "2l.bin", 0x1000000, 0x400000, CRC(1a7d51e9) SHA1(0722d89865cc2c9acda899bacb2787481c16c01a) )
	ROM_LOAD32_WORD( "2h.bin", 0x1000002, 0x400000, CRC(bf866222) SHA1(c70f4b0ff3997bf94a89cd613a2877f062014393) )
	ROM_LOAD32_WORD( "3l.bin", 0x1800000, 0x400000, CRC(a8a646f7) SHA1(be96626f3a4c8eb81f0bb7d8ac1c4e6619be50c8) )
	ROM_LOAD32_WORD( "3h.bin", 0x1800002, 0x400000, CRC(8c32becd) SHA1(9a8ddda4c6c007bb5cd4abb11859a4b7f1b1d578) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )
	ROM_LOAD( "snd0.bin", 0x000000, 0x400000, CRC(c090d51a) SHA1(d229753b536209fe0da1985ca694fd1a73bc0f39) )
	ROM_LOAD( "snd1.bin", 0x400000, 0x400000, CRC(c24243b5) SHA1(2100d5d7d2e4b9ed90bde38cb61a5da09f00ce21) )
ROM_END

ROM_START( hgkairak )
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u23",   0x000002, 0x080000, CRC(1c1a034d) SHA1(1be7793b1f9a0a738519b4b4f663b247011870db) )
	ROM_LOAD32_WORD_SWAP( "1.u22",   0x000000, 0x080000, CRC(24b04aa2) SHA1(b63d02fc15f03b93a74f5549fad236939905e382) )
	ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x100000, CRC(83cff542) SHA1(0ea5717e0b9e6c27aaf61f7e4909ed9a353b4d3b) )

	ROM_REGION( 0x3000000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(f7472212) SHA1(6f6c1a75615f6a1df4d9bc97225b8e1422eb114a) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(30019d0f) SHA1(1b6690ead9941171086afc89d95292c40348a15b) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(f46d5002) SHA1(0ce47b1c6da1a8ec3fd341d903d6a3e0447529e2) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(210592b6) SHA1(d98c3df589f5a707043979ede44c397705c13d11) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(b98adf21) SHA1(592b609f665a8a6af169611d6dbe7580df22e0c8) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(8e3da1e1) SHA1(9001d7484ad85d1febf9c1b925445246dfd66419) )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, CRC(fa7ba4ed) SHA1(cfe7651a48549a15f3fa81c744cf9204cd4f6f9a) )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, CRC(a5d400ea) SHA1(a550b30fa854bab11389a81fb73479bec0f4a5ff) )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, CRC(76c10026) SHA1(9cb2f29d123065d62a42743307db3f949432e2d5) )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, CRC(799f0889) SHA1(8495a15a2b2bd5d7324264b388f3b7e5a7d36cd6) )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, CRC(4639ef36) SHA1(324ffcfa1b1b9def00c15f628c59cea1d09b031d) )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, CRC(549e9e9e) SHA1(90c1695c89c059852f8b4f714b3dfee006839b44) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(0e8e5fdf) SHA1(041e3118f7a838dcc9fb99a1028fb48a452ba1d9) )
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(d8057d2f) SHA1(51d96cc4e9da81cbd1e815c652707407e6c7c3ae) )
ROM_END

ROM_START( hotgmck3 )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(3b06a4a3) SHA1(7363c2867367ca92a20fcb5ee1a5f1afbd785c63) )
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(7aad6b24) SHA1(160dfac94002766709369aad66d3b1b11d35ee63) )
	ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x100000, CRC(316c3356) SHA1(4664465c3f88d655379235881f1142a7954c80fc) )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(d4bbd035) SHA1(525739eafa4574541b217707514b256af588a996) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(e8832b0b) SHA1(aa13c264964b1c48094a303b18407a7873c60267) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(08426cb2) SHA1(a66463a93580fa7b730df7d0b72176daf6d0a6f2) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(112c6eea) SHA1(45d00427cec4f8ba98dc2f5b1e0d13d0f7ef56d1) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(0f197cd4) SHA1(ea1f20260470a5bee1076c3c243d7f57aabdaaba) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(fc37808c) SHA1(c35c666aad123ea1a37a5b4d6d51e55204c18e02) )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, CRC(c4d094dc) SHA1(bce585e41c9892be84a9fbdbb6d0f6b9baecaccf) )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, CRC(ef0dad0a) SHA1(f98032a8b7c3e17b52761ba98e46bdc5e0fa032a) )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, CRC(5186790f) SHA1(e2e2beb0dee856ec089c6a380442b4a1f40901d9) )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, CRC(187a6f43) SHA1(eedcc58ba59ffb0ccd7925edeb4532f8de357ec1) )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, CRC(ecf151f3) SHA1(56277c5d675e579e2148961aadc06019a64f3367) )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, CRC(720bf4ec) SHA1(214fb544ddc229485ae2a5ec34f95af9fa423a80) )
	ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x400000, CRC(e490404d) SHA1(dc3429ed248e7954c27eb6f29b392a35131592ba) )
	ROM_LOAD32_WORD( "6h.u17", 0x3000002, 0x400000, CRC(7e8a141a) SHA1(3f53e422a3ad2f2a02d17b2368b2d455c614bc04) )
	ROM_LOAD32_WORD( "7l.u9",  0x3800000, 0x400000, CRC(2ec78fb2) SHA1(194e9833ab7057c2f83c581e722b41631d99fccc) )
	ROM_LOAD32_WORD( "7h.u18", 0x3800002, 0x400000, CRC(c1735612) SHA1(84e32d3249d57cdc8ea91780801eaa196c439895) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(d62a0dba) SHA1(d81e2e1251b62eca8cd4d8eec2515b2cf7d7ff0a) )
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(1df91fb4) SHA1(f0f2d2d717fbd16a67da9f0e21f288ceedef839f) )
ROM_END

ROM_START( hotgm4ev )
	/* main program */
	ROM_REGION( 0x500000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(3334c21e) SHA1(8d825448e40bc50d670ab8587a40df6b27ac918e) )
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(b1a1c643) SHA1(1912a2d231e97ffbe9b668ca7f25cf406664f3ba) )
    ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x400000, CRC(ad556d8e) SHA1(d3dc3c5cbe939b6fc28f861e4132c5485ba89f50) ) // no test

	ROM_REGION( 0x8000000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
    ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(f65986f7) SHA1(3824a7ea7f14ef3f319b07bd1224847131f6cac0) ) // ok
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(51fd07a9) SHA1(527097a376fc0ecb23aa8707eb5e029ec1126873) ) // ok
    ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(f59d21d7) SHA1(05a1b93f2926b419ff5e031a25fd227706d66050) ) // ok
    ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(60ea4797) SHA1(5da4a26496ac986ba0bdefde63040634775132dc) ) // ok
    ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(fbaf05e3) SHA1(ce929f1e87b05324e5e863bdb8baafef3d73f781) ) // ok
    ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(61281612) SHA1(88af33aa9348044c3620dd7a89e992f3812f45a8) ) // ok
    ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x400000, CRC(e2e1bd9f) SHA1(8078bbffd6bf3abae6a3b36ae26628eb53ee5dc2) ) // ok
    ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x400000, CRC(c4426542) SHA1(660e0fc3f641034cc0b2a8398951de0579b35d77) ) // ok
    ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x400000, CRC(7298a242) SHA1(f1987a466bc1cb569d1b5f918ef6690b24479b3b) ) // ok
    ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x400000, CRC(fe91b459) SHA1(18d0dc1c9f9103d505110dbc1a44f805597037d9) ) // ok
    ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x400000, CRC(cc714a7d) SHA1(933c08fb34b138279d5ee7783d1b4865b2d4520a) ) // ok
    ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x400000, CRC(2f149cf9) SHA1(598c3e606e3cd87938ccfc3c5e08d88c6044b393) ) // ok
    ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x400000, CRC(bfe97dfe) SHA1(f249258a620188d0d7c3858cfb36af5d67ba94b2) ) // ok
    ROM_LOAD32_WORD( "6h.u17", 0x3000002, 0x400000, CRC(3473052a) SHA1(897987422885a19a53a02257a9dde6348ec42f9f) ) // ok
    ROM_LOAD32_WORD( "7l.u9",  0x3800000, 0x400000, CRC(022a8a31) SHA1(a21dbf36f56e144f9817c7255866546367dda2f6) ) // ok
    ROM_LOAD32_WORD( "7h.u18", 0x3800002, 0x400000, CRC(77e47409) SHA1(0e1deb01dd1250c90fc3eed776becd51899f0b5f) ) // ok

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )
    ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(051e2fed) SHA1(ee8073332801982549b3c142fba114e27733a756) ) // ok
    ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(0de0232d) SHA1(c600fe1d3c6c05e451ae7ef249bb92e8fc9cec3a) ) // ok (but fails rom test due to banking error in emulation)
ROM_END

ROM_START( hotgmcki )
	ROM_REGION( 0x500000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(abc192dd) SHA1(674c2b8814319605c1b6221bbe18588a98dda093) )
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(8be896d0) SHA1(5d677dede4ec18cbfc54acae95fe0f10bfc4d566) )
    ROM_LOAD16_WORD_SWAP( "prog.u1", 0x100000, 0x200000, CRC(9017ae8e) SHA1(0879198606095a2d209df059538ce1c73460b30e) ) // no test
	ROM_RELOAD(0x300000,0x200000)

	/* Roms have to be mirrored with ROM_RELOAD for rom tests to pass */
	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_ERASEFF | ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x200000, CRC(58ae45eb) SHA1(76a23e79f2c772c5e85b8c15cf79f56b6f71fbc6) ) // ok
	ROM_RELOAD(                0x0400000, 0x200000 )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x200000, CRC(d7bbb929) SHA1(c505ad04cdafb84800099bbbb67c5f6b52212124) ) // ok
	ROM_RELOAD(                0x0400002, 0x200000 )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x200000, CRC(27576360) SHA1(ed9d6f5b9934e8ddae3f3ca146e99f42dbd495de) ) // ok
	ROM_RELOAD(                0x0c00000, 0x200000 )
    ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x200000, CRC(7439a63f) SHA1(c9a74e5e81a2c94ce7a9b3b487f54ac5b3635744) ) // ok
	ROM_RELOAD(                0x0c00002, 0x200000 )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x200000, CRC(fda64e24) SHA1(e8788b5f0e8b0f90c9942f9f4cbcee02be8afd09) ) // ok
	ROM_RELOAD(                0x1400000, 0x200000 )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x200000, CRC(8be54ea6) SHA1(311812e9d7815e80db05d9a94c277b2b37e7a589) ) // ok
	ROM_RELOAD(                0x1400002, 0x200000 )
	ROM_LOAD32_WORD( "3l.u5",  0x1800000, 0x200000, CRC(92507b3f) SHA1(15f0432a8061e886be6171ee90581ad8fe3d44ba) ) // ok
	ROM_RELOAD(                0x1c00000, 0x200000 )
	ROM_LOAD32_WORD( "3h.u14", 0x1800002, 0x200000, CRC(042bef5e) SHA1(b57ffa744a17be21681db3d5246025ea27067ae7) ) // ok
	ROM_RELOAD(                0x1c00002, 0x200000 )
	ROM_LOAD32_WORD( "4l.u6",  0x2000000, 0x200000, CRC(023b6d70) SHA1(7932d8648d1c2e90539955a6474d9755f02b4350) ) // ok
	ROM_RELOAD(                0x2400000, 0x200000 )
	ROM_LOAD32_WORD( "4h.u15", 0x2000002, 0x200000, CRC(9be7e8b1) SHA1(2200bb86b0a7b05a9387da14d8bdc571d0570d11) ) // ok
	ROM_RELOAD(                0x2400002, 0x200000 )
	ROM_LOAD32_WORD( "5l.u7",  0x2800000, 0x200000, CRC(7aa54306) SHA1(ec9ecc8dbe81679e0e7544cf5f331ca3eeee700a) ) // ok
	ROM_RELOAD(                0x2c00000, 0x200000 )
	ROM_LOAD32_WORD( "5h.u16", 0x2800002, 0x200000, CRC(e6b48e52) SHA1(1deb84fe96fe31ba33ddf833967ef332570b8fe5) ) // ok
	ROM_RELOAD(                0x2c00002, 0x200000 )
	ROM_LOAD32_WORD( "6l.u8",  0x3000000, 0x200000, CRC(dfe675e9) SHA1(a4ff934c4b0501be490a2ba3a0ef1d46bc1d68e7) ) // ok
	ROM_RELOAD(                0x3400000, 0x200000 )
	ROM_LOAD32_WORD( "6h.u17", 0x3000002, 0x200000, CRC(45919576) SHA1(63c509f8786ebd43ec24664bb2b829f4bf8acf5d) ) // ok
	ROM_RELOAD(                0x3400002, 0x200000 )
	ROM_LOAD32_WORD( "7l.u9",  0x3800000, 0x200000, CRC(cd3af598) SHA1(57c8048802264a4699e0e95b8deb25689afff237) ) // ok
	ROM_RELOAD(                0x3c00000, 0x200000 )
	ROM_LOAD32_WORD( "7h.u18", 0x3800002, 0x200000, CRC(a3fd4ae5) SHA1(31056e5f645984b85e9bc3767016a856ac0175f9) ) // ok
	ROM_RELOAD(                0x3c00002, 0x200000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(5f275f35) SHA1(c5952a16e9f0cee6fc990c234ccaa7ca577741bd) ) // ok
	ROM_LOAD( "snd1.u19", 0x400000, 0x400000, CRC(98608779) SHA1(a73c21f0f66c2af903e44a0a6a9f821b00615e7b) ) // ok (but fails rom test due to bad banking in service mode)
ROM_END


ROM_START( loderndf )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1b.u23", 0x000002, 0x080000, CRC(fae92286) SHA1(c3d3a50514fb9c0bbd3ffb5c4bfcc853dc1893d2) )
	ROM_LOAD32_WORD_SWAP( "2b.u22", 0x000000, 0x080000, CRC(fe2424c0) SHA1(48a329cfdf98da1a8701b430c159d470c0f5eca1) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, CRC(ccae855d) SHA1(1fc44e2a9d2ce2bca0a57e96140fbc80a0943141) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, CRC(7a146c59) SHA1(e4b30b5a826b8772d144d503e591d1d32783c016) )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, CRC(7a9cd21e) SHA1(dfb36625c2aae3e774ec2451051b7038e0767b6d) )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, CRC(78f40d0d) SHA1(243acb73a183a41a3e35a2c746ad31dd6fcd3ef4) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, CRC(2da3788f) SHA1(199d4d750a107cbdf8c16cd5b097171743769d9c) ) // Fails hidden rom test (banking problem?)
ROM_END

ROM_START( loderdfa )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "12.u23", 0x000002, 0x080000, CRC(661d372e) SHA1(c509c3ad9ca01e0f58bfc319b2738ecc36865ffd) )
	ROM_LOAD32_WORD_SWAP( "3.u22", 0x000000, 0x080000, CRC(0a63529f) SHA1(05dd7877041b69d46e41c5bddb877c083620294b) )

	ROM_REGION( 0x2000000, REGION_GFX1, ROMTEST_GFX )
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x800000, CRC(ccae855d) SHA1(1fc44e2a9d2ce2bca0a57e96140fbc80a0943141) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x800000, CRC(7a146c59) SHA1(e4b30b5a826b8772d144d503e591d1d32783c016) )
	ROM_LOAD32_WORD( "1l.u3",  0x1000000, 0x800000, CRC(7a9cd21e) SHA1(dfb36625c2aae3e774ec2451051b7038e0767b6d) )
	ROM_LOAD32_WORD( "1h.u12", 0x1000002, 0x800000, CRC(78f40d0d) SHA1(243acb73a183a41a3e35a2c746ad31dd6fcd3ef4) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x800000, CRC(2da3788f) SHA1(199d4d750a107cbdf8c16cd5b097171743769d9c) ) // Fails hidden rom test (banking problem?)
ROM_END

ROM_START( hotdebut )
	ROM_REGION( 0x100000, REGION_CPU1, 0)
	ROM_LOAD32_WORD_SWAP( "1.u23",   0x000002, 0x080000, CRC(0b0d0027) SHA1(f62c487a725439af035d2904d453d3c2f7a5649b) )
	ROM_LOAD32_WORD_SWAP( "2.u22",   0x000000, 0x080000, CRC(c3b5180b) SHA1(615cc1fd99a1e4634b04bb92a3c41f914644e903) )

	ROM_REGION( 0x1800000, REGION_GFX1, ROMTEST_GFX )	/* Sprites */
	ROM_LOAD32_WORD( "0l.u2",  0x0000000, 0x400000, CRC(15da9983) SHA1(a96dd048080b5bb5ce903b5f72b3c24e89e1bee3) )
	ROM_LOAD32_WORD( "0h.u11", 0x0000002, 0x400000, CRC(76d7b73f) SHA1(0682d4155ad61cab958d55b85914c69120d7d6fc) )
	ROM_LOAD32_WORD( "1l.u3",  0x0800000, 0x400000, CRC(76ea3498) SHA1(ab2fb4008cf1e2b48a81306386cdc463b3bb4783) )
	ROM_LOAD32_WORD( "1h.u12", 0x0800002, 0x400000, CRC(a056859f) SHA1(5821cbb263059a32c5b599da666fb59929d6326a) )
	ROM_LOAD32_WORD( "2l.u4",  0x1000000, 0x400000, CRC(9d2d1bb1) SHA1(33b41aa50be3040871b6dc6faee0bd99c5e46cd3) )
	ROM_LOAD32_WORD( "2h.u13", 0x1000002, 0x400000, CRC(a7753c4d) SHA1(adb33de478064cc9255d1bb5c63acc5d8bfbb8eb) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "snd0.u10", 0x000000, 0x400000, CRC(eef28aa7) SHA1(d10d3f62a2e4c2a8e5fccece9c272f8ead50e5ed) )
ROM_END

/* are these right? should i fake the counter return?
   'speedups / idle skipping isn't needed for 'hotgmck, hgkairak'
   as the core catches and skips the idle loops automatically'
*/

static READ32_HANDLER( loderndf_speedup_r )
{
/*
PC  :00001B3C: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B3E: ADD     #$01,R3
PC  :00001B40: MOV.L   R3,@R14
PC  :00001B42: MOV.L   @($54,PC),R1
PC  :00001B44: MOV.L   @R1,R2
PC  :00001B46: TST     R2,R2
PC  :00001B48: BT      $00001B3C
*/

	if (activecpu_get_pc()==0x00001B3E) cpu_spinuntil_int();
	return ps4_ram[0x000020/4];
}

static READ32_HANDLER( loderdfa_speedup_r )
{
/*
PC  :00001B48: MOV.L   @R14,R3  R14 = 0x6000020
PC  :00001B4A: ADD     #$01,R3
PC  :00001B4C: MOV.L   R3,@R14
PC  :00001B4E: MOV.L   @($54,PC),R1
PC  :00001B50: MOV.L   @R1,R2
PC  :00001B52: TST     R2,R2
PC  :00001B54: BT      $00001B48
*/

	if (activecpu_get_pc()==0x00001B4A) cpu_spinuntil_int();
	return ps4_ram[0x000020/4];
}

static READ32_HANDLER( hotdebut_speedup_r )
{
/*
PC  :000029EC: MOV.L   @R14,R2
PC  :000029EE: ADD     #$01,R2
PC  :000029F0: MOV.L   R2,@R14
PC  :000029F2: MOV.L   @($64,PC),R1
PC  :000029F4: MOV.L   @R1,R3
PC  :000029F6: TST     R3,R3
PC  :000029F8: BT      $000029EC
*/

	if (activecpu_get_pc()==0x000029EE) cpu_spinuntil_int();
	return ps4_ram[0x00001c/4];
}

static STATE_POSTLOAD( hotgmck_pcm_bank_postload )
{
	set_hotgmck_pcm_bank(machine, (FPTR)param);
}

static void install_hotgmck_pcm_bank(running_machine *machine)
{
	UINT8 *ymf_pcm = memory_region(machine, REGION_SOUND1);
	UINT8 *pcm_rom = memory_region(machine, REGION_SOUND2);

	memcpy(ymf_pcm, pcm_rom, 0x200000);

	ps4_io_select[0] = (ps4_io_select[0] & 0x00ffffff) | 0x32000000;
	set_hotgmck_pcm_bank(machine, 0);
	set_hotgmck_pcm_bank(machine, 1);

	memory_install_write32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x5800008, 0x580000b, 0, 0, hotgmck_pcm_bank_w );
	state_save_register_postload(machine, hotgmck_pcm_bank_postload, (void *)0);
	state_save_register_postload(machine, hotgmck_pcm_bank_postload, (void *)1);
}

static DRIVER_INIT( hotgmck )
{
	UINT8 *RAM = memory_region(machine, REGION_CPU1);
	memory_set_bankptr(1,&RAM[0x100000]);
	memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x5800000, 0x5800007, 0, 0, hotgmck_io32_r ); // Different Inputs
	install_hotgmck_pcm_bank(machine);	// Banked PCM ROM
}

static DRIVER_INIT( loderndf )
{
	memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x6000020, 0x6000023, 0, 0, loderndf_speedup_r );
}

static DRIVER_INIT( loderdfa )
{
	memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x6000020, 0x6000023, 0, 0, loderdfa_speedup_r );
}

static DRIVER_INIT( hotdebut )
{
	memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600001c, 0x600001f, 0, 0, hotdebut_speedup_r );
}


/*     YEAR  NAME      PARENT    MACHINE    INPUT     INIT      MONITOR COMPANY   FULLNAME FLAGS */

GAME( 1997, hotgmck,  0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick (Japan)", 0 )
GAME( 1998, hgkairak, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick Kairakuten (Japan)", 0 )
GAME( 1999, hotgmck3, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick 3 Digital Surfing (Japan)", 0 )
GAME( 2000, hotgm4ev, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Taisen Hot Gimmick 4 Ever (Japan)", 0 )
GAME( 2001, hotgmcki, 0,        ps4big,    hotgmck,  hotgmck,  ROT0,   "Psikyo", "Mahjong Hot Gimmick Integral (Japan)", 0 )
GAME( 2000, loderndf, 0,        ps4small,  loderndf, loderndf, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. B)", 0 )
GAME( 2000, loderdfa, loderndf, ps4small,  loderndf, loderdfa, ROT0,   "Psikyo", "Lode Runner - The Dig Fight (ver. A)", 0 )
GAME( 2000, hotdebut, 0,        ps4small,  hotdebut, hotdebut, ROT0,   "Psikyo / Moss", "Quiz de Idol! Hot Debut (Japan)", 0 )
