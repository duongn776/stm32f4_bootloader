"""
STM32F407 bootloader host (legacy niekiran protocol).
Spec: bootloader_pj/document/PROJECT_BRIEF.md, section 17.

Usage:
    python STM32_Programmer_V1.py             interactive menu
    python STM32_Programmer_V1.py selftest    check CRC, no board needed
"""
import os
import sys
import time

import serial
from serial.tools import list_ports

#----------------------------- Protocol ----------------------------------------

BL_ACK  = 0xA5
BL_NACK = 0x7F

COMMAND_BL_GET_VER              = 0x51
COMMAND_BL_GET_HELP             = 0x52
COMMAND_BL_GET_CID              = 0x53
COMMAND_BL_GET_RDP_STATUS       = 0x54
COMMAND_BL_GO_TO_ADDR           = 0x55
COMMAND_BL_FLASH_ERASE          = 0x56
COMMAND_BL_MEM_WRITE            = 0x57
COMMAND_BL_EN_R_W_PROTECT       = 0x58
COMMAND_BL_MEM_READ             = 0x59
COMMAND_BL_READ_SECTOR_P_STATUS = 0x5A
COMMAND_BL_OTP_READ             = 0x5B
COMMAND_BL_DIS_R_W_PROTECT      = 0x5C

# Status byte returned by FLASH_ERASE / MEM_WRITE
Flash_HAL_OK = 0x00
FLASH_STATUS = {
    0x00: "FLASH_HAL_OK",
    0x01: "FLASH_HAL_ERROR (or ADDR_INVALID, firmware uses 1 for both)",
    0x02: "FLASH_HAL_BUSY",
    0x03: "FLASH_HAL_TIMEOUT",
    0x04: "FLASH_HAL_INV_SECTOR",
}

# Supported bootloader commands, matched against bootloader_pj/Core/Src/main.c.
# firmware: "ok" = works, "unsafe" = works but dangerous,
#           "stub" = handler is empty and never replies (not offered in the menu).
SUPPORTED_COMMANDS = {
    COMMAND_BL_GET_VER:              ("BL_GET_VER",              "ok",     "Bootloader version"),
    COMMAND_BL_GET_HELP:             ("BL_GET_HELP",             "ok",     "List of opcodes reported by the board"),
    COMMAND_BL_GET_CID:              ("BL_GET_CID",              "ok",     "Chip ID, expect 0x413 (STM32F407)"),
    COMMAND_BL_GET_RDP_STATUS:       ("BL_GET_RDP_STATUS",       "ok",     "Flash read protection level"),
    COMMAND_BL_GO_TO_ADDR:           ("BL_GO_TO_ADDR",           "unsafe", "Blind jump, no MSP/VTOR setup"),
    COMMAND_BL_FLASH_ERASE:          ("BL_FLASH_ERASE",          "unsafe", "Host blocks mass erase and sectors 0-1"),
    COMMAND_BL_MEM_WRITE:            ("BL_MEM_WRITE",            "unsafe", "Host blocks writes into the bootloader"),
    COMMAND_BL_EN_R_W_PROTECT:       ("BL_EN_R_W_PROTECT",       "stub",   "Firmware handler empty"),
    COMMAND_BL_MEM_READ:             ("BL_MEM_READ",             "ok",     "Read 2..200 bytes"),
    COMMAND_BL_READ_SECTOR_P_STATUS: ("BL_READ_SECTOR_P_STATUS", "stub",   "Firmware handler empty"),
    COMMAND_BL_OTP_READ:             ("BL_OTP_READ",             "stub",   "Firmware handler empty, removed by brief"),
    COMMAND_BL_DIS_R_W_PROTECT:      ("BL_DIS_R_W_PROTECT",      "stub",   "Firmware handler empty"),
}

#----------------------------- Memory map (copy of common/boot_config.h) --------

BOOT_BL_END          = 0x08008000   # sectors 0-1 = bootloader, never erase/write
BOOT_SLOT_A_ADDR     = 0x08020000
BOOT_SLOT_A_END      = 0x08080000   # exclusive
BOOT_APP_VECTOR_ADDR = 0x08020200   # default write address for the app .bin
BOOT_IMG_MAX_SIZE    = 392704       # 0x5FE00
FLASH_NUM_SECTORS    = 12
DEV_ID_F407          = 0x413

#----------------------------- Limits ------------------------------------------

DEFAULT_TIMEOUT_S          = 2
ERASE_TIMEOUT_PER_SECTOR_S = 3      # 128 KB sector erase takes 1-2 s
MEM_WRITE_CHUNK            = 128
MEM_READ_MIN               = 2      # a 1-byte reply is ambiguous with "invalid address"
MEM_READ_MAX               = 200

verbose_mode  = False               # True: print every packet sent
bin_file_name = "user_application.bin"
ser = None

#----------------------------- CRC ---------------------------------------------

def get_crc(buff, length):
    # Same as firmware bootloader_verify_crc(): each byte is fed to the STM32 CRC unit
    # as one 32-bit word (CRC-32/MPEG-2, poly 0x04C11DB7, init 0xFFFFFFFF).
    crc = 0xFFFFFFFF
    for data in buff[0:length]:
        crc ^= data
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc

def crc_selftest():
    # Known-good vectors, brief section 17.1
    assert get_crc([0x05, 0x51], 2) == 0x7CABE9E7
    assert get_crc([0x05, 0x52], 2) == 0x71E8CF3E
    assert get_crc([0x05, 0x53], 2) == 0x7529D289
    assert get_crc([0x0A, 0x59, 0x00, 0x02, 0x02, 0x08, 0x10], 7) == 0x9B7C0F94

#----------------------------- Serial Port -------------------------------------

def serial_ports():
    return [p.device for p in list_ports.comports()]

def Serial_Port_Configuration(port):
    global ser
    try:
        ser = serial.Serial(port, 115200, timeout=DEFAULT_TIMEOUT_S)
    except serial.SerialException:
        print("\n   Oops! That was not a valid port")
        ports = serial_ports()
        if not ports:
            print("\n   No ports Detected")
        else:
            print("\n   Here are some available ports on your PC. Try Again!")
            print("\n   ", ports)
        return -1
    print("\n   Port Open Success")
    return 0

def read_serial_port(length):
    return ser.read(length)

def Close_serial_port():
    if ser is not None and ser.is_open:
        ser.close()

def purge_serial_port():
    ser.reset_input_buffer()

def send_command(command_code, payload=b""):
    """Send [len_to_follow][cmd][payload][crc32 LE] in one write."""
    packet = bytearray([1 + len(payload) + 4, command_code]) + payload
    packet += get_crc(packet, len(packet)).to_bytes(4, "little")
    if verbose_mode:
        print("\n   TX:", packet.hex(" "))
    ser.write(packet)

#----------------------------- Reply processing --------------------------------

def reply_complete(value, length):
    if len(value) != length:
        print("\n   Timeout : expected {} reply bytes, got {}".format(length, len(value)))
        return False
    return True

def print_supported_commands(board_list=None):
    print("\n   Code  Command                    Firmware  Board  Note")
    print("   ----  -------------------------  --------  -----  ---------------------------------")
    for code, (name, status, note) in SUPPORTED_COMMANDS.items():
        if board_list is None:
            on_board = "-"
        else:
            on_board = "yes" if code in board_list else "no"
        print("   {:#04x}  {:<25}  {:<8}  {:>5}  {}".format(code, name, status, on_board, note))
    if board_list is not None:
        unknown = [c for c in board_list if c not in SUPPORTED_COMMANDS]
        if unknown:
            print("\n   Board reports unknown opcodes:", " ".join(hex(c) for c in unknown))

def process_COMMAND_BL_GET_VER(length):
    value = read_serial_port(length)
    if reply_complete(value, length):
        print("\n   Bootloader Ver. : ", hex(value[0]))

def process_COMMAND_BL_GET_HELP(length):
    value = read_serial_port(length)
    if reply_complete(value, length):
        print("\n   Board reports :", " ".join(hex(x) for x in value))
        print_supported_commands(list(value))

def process_COMMAND_BL_GET_CID(length):
    value = read_serial_port(length)
    if not reply_complete(value, length):
        return
    chip_id = int.from_bytes(value[0:2], "little")
    print("\n   Chip Id. : ", hex(chip_id))
    if chip_id == DEV_ID_F407:
        print("   -> STM32F405/407 (OK)")
    else:
        print("   -> WARNING: expected {:#x} (STM32F407)".format(DEV_ID_F407))

def process_COMMAND_BL_GET_RDP_STATUS(length):
    value = read_serial_port(length)
    if not reply_complete(value, length):
        return
    rdp = value[0]
    if rdp == 0xAA:
        level = "Level 0 (no protection)"
    elif rdp == 0xCC:
        level = "Level 2 (chip locked, irreversible)"
    else:
        level = "Level 1 (read protection)"
    print("\n   RDP Status : ", hex(rdp), "->", level)

def process_COMMAND_BL_GO_TO_ADDR(length):
    value = read_serial_port(length)
    if reply_complete(value, length):
        print("\n   Address Status : ", hex(value[0]), "(0 = valid, 1 = invalid)")

def process_COMMAND_BL_FLASH_ERASE(length):
    value = read_serial_port(length)
    if reply_complete(value, length):
        print("\n   Erase Status:", FLASH_STATUS.get(value[0], "UNKNOWN_ERROR_CODE"))

def process_COMMAND_BL_MEM_WRITE(length):
    value = read_serial_port(length)
    if not reply_complete(value, length):
        return -2
    if value[0] != Flash_HAL_OK:
        print("\n   Write_status:", FLASH_STATUS.get(value[0], "UNKNOWN_ERROR"))
    return value[0]

def process_COMMAND_BL_MEM_READ(length):
    value = read_serial_port(length)
    if not reply_complete(value, length):
        return
    # Requests are >= 2 bytes, so a 1-byte reply can only mean "invalid address"
    if length == 1:
        print("\n   Read_status: INVALID ADDRESS (code {:#04x})".format(value[0]))
        return
    print("\n   Read data ({} byte):".format(length))
    for offset in range(0, length, 16):
        line = value[offset:offset + 16]
        print("   +{:03x}: {}".format(offset, line.hex(" ")))

REPLY_HANDLERS = {
    COMMAND_BL_GET_VER:        process_COMMAND_BL_GET_VER,
    COMMAND_BL_GET_HELP:       process_COMMAND_BL_GET_HELP,
    COMMAND_BL_GET_CID:        process_COMMAND_BL_GET_CID,
    COMMAND_BL_GET_RDP_STATUS: process_COMMAND_BL_GET_RDP_STATUS,
    COMMAND_BL_GO_TO_ADDR:     process_COMMAND_BL_GO_TO_ADDR,
    COMMAND_BL_FLASH_ERASE:    process_COMMAND_BL_FLASH_ERASE,
    COMMAND_BL_MEM_WRITE:      process_COMMAND_BL_MEM_WRITE,
    COMMAND_BL_MEM_READ:       process_COMMAND_BL_MEM_READ,
}

def wait_for_ack_or_nack():
    """Return BL_ACK / BL_NACK, or None on timeout.

    The firmware prints debug text (printmsg) on the same UART2, sometimes before
    the ACK. Every byte that is not ACK/NACK is collected and shown as board text.
    """
    junk = bytearray()
    deadline = time.monotonic() + ser.timeout
    result = None
    while time.monotonic() < deadline:
        b = read_serial_port(1)
        if not b:
            continue
        if b[0] in (BL_ACK, BL_NACK):
            result = b[0]
            break
        junk += b
    if junk:
        text = junk.decode("ascii", errors="replace").strip()
        print("\n   [board text] " + text)
        if "user application" in text:
            print("   -> Board is running the APP. Hold the USER button while pressing reset.")
    return result

def read_bootloader_reply(command_code):
    """Return 0 = OK, -1 = NACK, -2 = timeout, -3 = command reported an error."""
    first = wait_for_ack_or_nack()
    if first is None:
        print("\n   Timeout : Bootloader not responding")
        return -2
    if first == BL_NACK:
        print("\n   CRC: FAIL")
        return -1

    length = read_serial_port(1)
    if not length:
        print("\n   Timeout : ACK received but no length byte")
        return -2
    len_to_follow = length[0]
    if verbose_mode:
        print("\n   CRC : SUCCESS Len :", len_to_follow)

    result = REPLY_HANDLERS[command_code](len_to_follow)
    if command_code == COMMAND_BL_MEM_WRITE and result != Flash_HAL_OK:
        return -3
    return 0

#----------------------------- Input helpers -----------------------------------

def ask_int(prompt, base=10, default=None):
    """Return the typed number, `default` on empty input, or None if invalid."""
    text = input(prompt).strip()
    if not text and default is not None:
        return default
    try:
        return int(text, base)
    except ValueError:
        print("\n   Invalid number")
        return None

def confirm(prompt):
    return input(prompt).strip().lower() == "yes"

#----------------------------- Commands with arguments -------------------------

def go_to_addr():
    print("\n   WARNING: firmware jumps without setting MSP/VTOR. Prefer resetting the board.")
    if not confirm("   Type 'yes' to continue: "):
        print("\n   Command dropped")
        return 0
    go_address = ask_int("\n   Please enter 4 bytes go address in hex: ", 16)
    if go_address is None:
        return 0
    send_command(COMMAND_BL_GO_TO_ADDR, go_address.to_bytes(4, "little"))
    return read_bootloader_reply(COMMAND_BL_GO_TO_ADDR)

def flash_erase():
    print("\n   Sectors: 0-1 bootloader | 2-3 metadata | 4 reserved | 5-7 slot A | 8-10 slot B | 11 scratch")
    sector_num = ask_int("\n   Enter first sector number (decimal, 2-11): ")
    nsec = ask_int("\n   Enter number of sectors to erase: ")
    if sector_num is None or nsec is None:
        return 0

    # Host-side safety rules, brief section 17.4
    if nsec < 1 or sector_num < 0 or sector_num + nsec > FLASH_NUM_SECTORS:
        print("\n   Invalid range: sectors must stay inside 0-11 (mass erase is blocked)")
        return 0
    if sector_num <= 1:
        print("\n   Blocked: sectors 0-1 contain the bootloader")
        return 0
    if sector_num <= 4 and not confirm("\n   Sectors 2-4 hold metadata/reserved data. Type 'yes' to erase anyway: "):
        print("\n   Command dropped")
        return 0

    send_command(COMMAND_BL_FLASH_ERASE, bytes([sector_num, nsec]))
    ser.timeout = nsec * ERASE_TIMEOUT_PER_SECTOR_S + DEFAULT_TIMEOUT_S
    print("\n   Erasing, please wait up to {} s ...".format(ser.timeout))
    try:
        return read_bootloader_reply(COMMAND_BL_FLASH_ERASE)
    finally:
        ser.timeout = DEFAULT_TIMEOUT_S

def mem_write():
    global bin_file_name
    name = input("\n   Enter .bin file [{}]: ".format(bin_file_name)).strip()
    if name:
        bin_file_name = name
    if not os.path.isfile(bin_file_name):
        print("\n   File not found:", bin_file_name)
        return 0
    with open(bin_file_name, "rb") as f:
        data = f.read()

    base = ask_int("\n   Enter the memory write address [{:#010x}]: ".format(BOOT_APP_VECTOR_ADDR),
                   16, BOOT_APP_VECTOR_ADDR)
    if base is None:
        return 0

    # Host-side safety rules, brief section 17.4
    end = base + len(data)
    if base < BOOT_BL_END:
        print("\n   Blocked: address is inside the bootloader (< {:#010x})".format(BOOT_BL_END))
        return 0
    if (base < BOOT_SLOT_A_ADDR or end > BOOT_SLOT_A_END) and not confirm(
            "\n   Range {:#010x}-{:#010x} is outside slot A. Type 'yes' to continue: ".format(base, end - 1)):
        print("\n   Command dropped")
        return 0
    if base == BOOT_APP_VECTOR_ADDR and len(data) > BOOT_IMG_MAX_SIZE:
        print("\n   Blocked: file is {} B, app max is {} B".format(len(data), BOOT_IMG_MAX_SIZE))
        return 0
    print("\n   Reminder: target sectors must be erased first (slot A = erase sector 5, count 3)")

    for offset in range(0, len(data), MEM_WRITE_CHUNK):
        chunk = data[offset:offset + MEM_WRITE_CHUNK]
        address = base + offset
        send_command(COMMAND_BL_MEM_WRITE, address.to_bytes(4, "little") + bytes([len(chunk)]) + chunk)
        ret = read_bootloader_reply(COMMAND_BL_MEM_WRITE)
        if ret != 0:
            print("\n   Write stopped at {:#010x}".format(address))
            return ret
        print("\r   Written {}/{} bytes".format(offset + len(chunk), len(data)), end="")
    print("\n   Write done")
    return 0

def mem_read():
    mem_add = ask_int("\n   Enter the memory read address (hex): ", 16)
    read_length = ask_int("\n   Enter the number of bytes to read ({}-{}): ".format(MEM_READ_MIN, MEM_READ_MAX))
    if mem_add is None or read_length is None:
        return 0
    if not 0 <= mem_add <= 0xFFFFFFFF:
        print("\n   Invalid address")
        return 0
    if not MEM_READ_MIN <= read_length <= MEM_READ_MAX:
        print("\n   Invalid read length (must be {}-{})".format(MEM_READ_MIN, MEM_READ_MAX))
        return 0
    send_command(COMMAND_BL_MEM_READ, mem_add.to_bytes(4, "little") + bytes([read_length]))
    return read_bootloader_reply(COMMAND_BL_MEM_READ)

#----------------------------- Menu --------------------------------------------

# Commands without arguments: menu number -> opcode
SIMPLE_COMMANDS = {
    1: COMMAND_BL_GET_VER,
    2: COMMAND_BL_GET_HELP,
    3: COMMAND_BL_GET_CID,
    4: COMMAND_BL_GET_RDP_STATUS,
}

MENU = [
    (1, "BL_GET_VER"),
    (2, "BL_GET_HELP"),
    (3, "BL_GET_CID"),
    (4, "BL_GET_RDP_STATUS"),
    (5, "BL_GO_TO_ADDR  (careful)"),
    (6, "BL_FLASH_ERASE"),
    (7, "BL_MEM_WRITE"),
    (8, "BL_MEM_READ"),
    (9, "SUPPORTED_COMMANDS (table)"),
    (0, "MENU_EXIT"),
]

def decode_menu_command_code(command):
    ret_value = 0
    if command == 0:
        print("\n   Exiting...!")
        Close_serial_port()
        raise SystemExit
    elif command in SIMPLE_COMMANDS:
        opcode = SIMPLE_COMMANDS[command]
        print("\n   Command == >", SUPPORTED_COMMANDS[opcode][0])
        send_command(opcode)
        ret_value = read_bootloader_reply(opcode)
    elif command == 5:
        print("\n   Command == > BL_GO_TO_ADDR")
        ret_value = go_to_addr()
    elif command == 6:
        print("\n   Command == > BL_FLASH_ERASE")
        ret_value = flash_erase()
    elif command == 7:
        print("\n   Command == > BL_MEM_WRITE")
        ret_value = mem_write()
    elif command == 8:
        print("\n   Command == > BL_MEM_READ")
        ret_value = mem_read()
    elif command == 9:
        print_supported_commands()
    else:
        print("\n   Please input valid command code")

    if ret_value == -2:
        print("\n   Reset the board and Try Again !")

def print_menu():
    print("\n +==========================================+")
    print(" |               Menu                       |")
    print(" |         STM32F407 BootLoader v1          |")
    print(" +==========================================+")
    print("\n   Which BL command do you want to send ??\n")
    for number, label in MENU:
        print("   {:<36} --> {}".format(label, number))

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "selftest":
        crc_selftest()
        print("selftest OK")
        raise SystemExit

    print("Available ports:", serial_ports() or "none")
    name = input("Enter the Port Name of your device(Ex: COM3):")
    if Serial_Port_Configuration(name) < 0:
        decode_menu_command_code(0)

    while True:
        print_menu()
        command_code = input("\n   Type the command code here :")
        if not command_code.isdigit():
            print("\n   Please Input valid code shown above")
        else:
            decode_menu_command_code(int(command_code))
        input("\n   Press Enter to continue :")
        purge_serial_port()
