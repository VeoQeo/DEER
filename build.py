#!/usr/bin/env python3


# TODO: --clean-git
# TODO: clean project for upload on github
import os
import sys
import subprocess
import argparse
from pathlib import Path
import json
import time


def run(cmd, check=True, shell=False, cwd=None):
    """Универсальная обёртка для выполнения команд."""
    print(f"[RUN] {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    result = subprocess.run(cmd, shell=shell, check=check, cwd=cwd)
    return result


class Builder:
    def __init__(self, name=None, version=None):
        self.ensure_config()  # ← Добавлено

        self.ARCH = "x86_64"
        self.OUTPUT = "kernel"
        self.KERNEL_DIR = Path("kernel")
        self.BUILD_DIR = self.KERNEL_DIR / f"bin-{self.ARCH}"
        self.OBJ_DIR = self.KERNEL_DIR / f"obj-{self.ARCH}"
        self.LINKER_SCRIPT = self.KERNEL_DIR / "linker-scripts" / f"{self.ARCH}.lds"

        TOOLS_DIR = Path("limine-tools")
        self.LIMINE_DIR = TOOLS_DIR / "limine"
        self.OVMF_DIR = TOOLS_DIR / "ovmf"
        self.OVMF_FILE = self.OVMF_DIR / f"ovmf-code-{self.ARCH}.fd"

        self.ISO_DIR = Path("iso_root")
        self.ISO_FILE = Path("deer-v0.0.1.iso")
        self.HDD_FILE = Path("deer-v0.0.1.hdd")

        self.QEMU = f"qemu-system-{self.ARCH}"

        # Загружаем конфиг
        with open("os-config.json", 'r', encoding='utf-8') as f:
            config = json.load(f)
        self.OS_NAME = name or config.get("name", "DEER")
        self.OS_VERSION = version or config.get("version", "v0.0.1")

        # Генерация имени ISO
        timestamp = time.strftime("%Y%m%d-%H%M%S")
        safe_name = "".join(c for c in self.OS_NAME if c.isalnum() or c in "._-")
        safe_version = "".join(c for c in self.OS_VERSION if c.isalnum() or c in "._-")
        self.FINAL_ISO_NAME = f"{safe_name}.{safe_version}-{timestamp}.iso"

        self.DEMO_ISO_DIR = Path("demo_iso")
        self.DEMO_ISO_DIR.mkdir(exist_ok=True)

    def clean(self):
        """Очистить временные файлы сборки и ISO."""
        cleaned = []
        if self.BUILD_DIR.exists():
            run(["rm", "-rf", str(self.BUILD_DIR)])
            cleaned.append(str(self.BUILD_DIR))
        if self.OBJ_DIR.exists():
            run(["rm", "-rf", str(self.OBJ_DIR)])
            cleaned.append(str(self.OBJ_DIR))
        # НЕ очищаем ISO_DIR - оставляем для пользовательских файлов
        if self.ISO_FILE.exists():
            self.ISO_FILE.unlink()
            cleaned.append(str(self.ISO_FILE))

        if cleaned:
            print(f"[OK] Очищено: {', '.join(cleaned)}")
        else:
            print("[OK] Нечего чистить.")

    def distclean(self):
        """Полная очистка: включая зависимости."""     
        self.clean()
        tools_dir = Path("limine-tools")
        if tools_dir.exists():
            run(["rm", "-rf", str(tools_dir)])
        deps_flag = self.KERNEL_DIR / ".deps-obtained"
        if deps_flag.exists():
            deps_flag.unlink()
        print("[OK] Полная очистка завершена.")

   # Clean for github
    def gitclean(self):
        """Очистка перед коммитом."""
        self.clean()
        # It's assumed 'self.clean()' is another method that works correctly

        tools_dir = Path("kernel/linker-scripts")

        os_tree_deer = Path("OS-TREE.txt")

        limine_tools_dir = Path("limine-tools")

        demo_iso_dir = Path("demo_iso")
        if demo_iso_dir.exists():
            try:
                run(["rm", "-rf", str(demo_iso_dir)])
                print(f"[OK] Удалена директория {demo_iso_dir}")
            except OSError as e:
                printf(f"[ERROR] Не удалось удалить {demo_iso_dir}: {e.strerror}")

        if limine_tools_dir.exists():
            try:
                run(["rm", "-rf", str(limine_tools_dir)])
                print(f"[OK]: Директория {limine_tools_dir} успешно удалена")
            except OSError as e:
                printf(f"[ERROR]: Не удалось удалить {limine_tools_dir}")

        if os_tree_deer.exists():
            try:
                run(["rm", str(os_tree_deer)])
                print(f"[OK] Удален файл {os_tree_deer}")
            except OSError as e:
                printf(f"[ERROR] Не удалось удалить {os_tree_deer}: {e.strerror}")
        if tools_dir.exists():
            try:
                run(["rm", "-rf", str(tools_dir)])
                print(f"[OK] Удалена директория {tools_dir}") # Added consistent print
            except OSError as e:
                printf(f"[ERROR] Не удалось удалить  {tools_dir}: {e.strerror}")

        deps_flag = self.KERNEL_DIR / ".deps-obtained"
        if deps_flag.exists():
            try:
                deps_flag.unlink()
                print(f"[OK] Удален файл флага зависимостей: {deps_flag.name}") # Added descriptive print
            except OSError as e:
                printf(f"[ERROR] Не удалось удалить {deps_flag}: {e.strerror}")

    print("[OK] Очистка завершена.") # More formal message

    def ensure_deps(self):
        deps_flag = self.KERNEL_DIR / ".deps-obtained"
        if not deps_flag.exists():
            print("[!] Зависимости не установлены. Запускаем get-deps...")
            run(["./get-deps"], cwd=str(self.KERNEL_DIR))
            deps_flag.touch()
    
    def ensure_config(self):
        """Создаёт os-config.json, если он не существует."""
        config_path = Path("os-config.json")
        if config_path.exists():
            return

        print(f"[*] Создание конфигурационного файла {config_path}...")

        config_data = {
            "name": "DEER",
            "version": "v0.0.1",
            "description": "A 64-bit operating system for extreme conditions.",
            "author": "VeoQeo",
            "arch": "x86_64",
            "bootloader": "limine",
            "website": None,
            "license": "MIT"
        }

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(config_data, f, indent=4, ensure_ascii=False)

        print(f"[OK] Конфигурация сохранена в {config_path}")

    def ensure_linker_scripts(self):
        """Создаёт папку linker-scripts и файл x86_64.lds, если их нет."""
        script_dir = self.KERNEL_DIR / "linker-scripts"
        script_file = script_dir / "x86_64.lds"

        if not script_file.exists():
            print(f"[*] Создание {script_file}...")

            lds_content = '''OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernel_main)

PHDRS
{
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = 0xffffffff80000000;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .note.gnu.build-id : {
        *(.note.gnu.build-id)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
'''
            script_dir.mkdir(exist_ok=True)
            with open(script_file, 'w', encoding='utf-8') as f:
                f.write(lds_content.strip() + '\n')
            print(f"[OK] Файл {script_file} создан.")
        else:
            print(f"[OK] Линкер-скрипт уже существует: {script_file}")

    def build_kernel(self):
        print("[*] Сборка ядра для x86_64...")

        # Гарантируем наличие linker-scripts/x86_64.lds
        self.ensure_linker_scripts()

        from glob import glob
        c_files = [Path(f) for f in glob("src/**/*.c", recursive=True, root_dir=self.KERNEL_DIR)]
        s_files = [Path(f) for f in glob("src/**/*.S", recursive=True, root_dir=self.KERNEL_DIR)]
        asm_files = [Path(f) for f in glob("src/**/*.asm", recursive=True, root_dir=self.KERNEL_DIR)]

        arch_c = [Path(f) for f in glob("src/arch/x86_64/**/*.c", recursive=True, root_dir=self.KERNEL_DIR)]
        arch_s = [Path(f) for f in glob("src/arch/x86_64/**/*.S", recursive=True, root_dir=self.KERNEL_DIR)]
        arch_asm = [Path(f) for f in glob("src/arch/x86_64/**/*.asm", recursive=True, root_dir=self.KERNEL_DIR)]

        all_c = c_files + arch_c
        all_s = s_files + arch_s
        all_asm = asm_files + arch_asm

        objects = []

        CC = os.getenv("CC", "gcc")
        LD = os.getenv("LD", "ld")
        NASM = "nasm"

        # Основные флаги компиляции (без SSE)
        CFLAGS = [
            "-g", "-O2", "-pipe", "-Wall", "-Wextra", "-std=gnu11",
            "-ffreestanding", "-fno-stack-protector",
            "-fno-stack-check", "-fno-lto", "-fno-PIC",
            "-ffunction-sections", "-fdata-sections",
            "-m64", "-march=x86-64", "-mabi=sysv",
            "-mno-80387", "-mno-mmx", "-mno-sse", "-mno-sse2",
            "-mno-red-zone", "-mcmodel=kernel"
        ]

        # Специальные флаги для SIMD файлов и kernel.c (с SSE)
        SIMD_CFLAGS = [
            "-g", "-O2", "-pipe", "-Wall", "-Wextra", "-std=gnu11",
            "-ffreestanding", "-fno-stack-protector",
            "-fno-stack-check", "-fno-lto", "-fno-PIC",
            "-ffunction-sections", "-fdata-sections",
            "-m64", "-march=x86-64", "-mabi=sysv",
            "-mno-80387", "-mno-mmx", 
            # ВКЛЮЧАЕМ SSE ДЛЯ SIMD ФАЙЛОВ И KERNEL.C:
            "-msse", "-msse2",
            "-mno-red-zone", "-mcmodel=kernel"
        ]

        CPPFLAGS = [
            f"-I{self.KERNEL_DIR}/src",
            f"-I{self.KERNEL_DIR}/../limine-tools/limine-protocol/include",
            f"-isystem{self.KERNEL_DIR}/../limine-tools/freestnd-c-hdrs/include",
            "-nostdinc",
            "-DLIMINE_API_REVISION=3",
            "-MMD", "-MP"
        ]

        LDFLAGS = [
            "-nostdlib", "-static",
            "-z", "max-page-size=0x1000",
            "--gc-sections",
            f"-T{self.LINKER_SCRIPT}"
        ]

        os.makedirs(self.OBJ_DIR, exist_ok=True)

        # Компиляция .c файлов
        for src in all_c:
            rel_src = Path("src") / src
            obj = self.OBJ_DIR / rel_src.with_suffix(".c.o")
            os.makedirs(obj.parent, exist_ok=True)
            
            # Выбираем флаги в зависимости от файла
            src_str = str(src)
            if "simd" in src_str.lower() or "kernel.c" in src_str:
                # Для SIMD файлов и kernel.c используем специальные флаги
                print(f"[*] Компиляция с SSE: {src}")
                cmd = [CC] + SIMD_CFLAGS + CPPFLAGS + ["-c", str(self.KERNEL_DIR / src), "-o", str(obj)]
            else:
                # Для всех остальных файлов - обычные флаги
                cmd = [CC] + CFLAGS + CPPFLAGS + ["-c", str(self.KERNEL_DIR / src), "-o", str(obj)]
            
            run(cmd)
            objects.append(obj)

        # Компиляция .S файлов (ассемблер) - всегда без SSE
        for src in all_s:
            rel_src = Path("src") / src
            obj = self.OBJ_DIR / rel_src.with_suffix(".S.o")
            os.makedirs(obj.parent, exist_ok=True)
            cmd = [CC] + CFLAGS + CPPFLAGS + ["-c", str(self.KERNEL_DIR / src), "-o", str(obj)]
            run(cmd)
            objects.append(obj)

        # Компиляция .asm через NASM
        NASMFLAGS = ["-g", "-F", "dwarf", "-Wall", "-f", "elf64"]
        for src in all_asm:
            rel_src = Path("src") / src
            obj = self.OBJ_DIR / rel_src.with_suffix(".asm.o")
            os.makedirs(obj.parent, exist_ok=True)
            cmd = [NASM] + NASMFLAGS + [str(self.KERNEL_DIR / src), "-o", str(obj)]
            run(cmd)
            objects.append(obj)

        # Конвертируем font.psf в объектный файл
        font_psf = self.KERNEL_DIR / "src" / "font.psf"
        font_o = self.OBJ_DIR / "font.o"

        if font_psf.exists():
            print("[*] Конвертация font.psf -> font.o...")
            run([
                "objcopy",
                "-I", "binary",
                "-O", "elf64-x86-64",
                "-B", "i386",
                "--redefine-sym", "_binary_kernel_src_font_psf_start=_binary_font_psf_start",
                "--redefine-sym", "_binary_kernel_src_font_psf_end=_binary_font_psf_end",
                "--redefine-sym", "_binary_kernel_src_font_psf_size=_binary_font_psf_size",
                str(font_psf),
                str(font_o)
            ])
            objects.append(font_o)
        else:
            print(f"[!] Файл шрифта не найден: {font_psf}")
            sys.exit(1)

        # Линковка
        kernel_out = self.BUILD_DIR / self.OUTPUT
        os.makedirs(self.BUILD_DIR, exist_ok=True)
        link_cmd = [LD] + LDFLAGS + [str(obj) for obj in objects] + ["-o", str(kernel_out)]
        run(link_cmd)
        print(f"[OK] Ядро собрано: {kernel_out}")

    def clone_limine(self):
        if not self.LIMINE_DIR.parent.exists():
            self.LIMINE_DIR.parent.mkdir(parents=True, exist_ok=True)

        if not self.LIMINE_DIR.exists():
            print("[*] Клонирование Limine binary release в limine-tools/limine...")
            run([
                "git", "clone",
                "--branch", "v10.x-binary",
                "--depth", "1",
                "https://codeberg.org/Limine/Limine.git",
                str(self.LIMINE_DIR)
            ])
        run(["make"], cwd=self.LIMINE_DIR)

    def download_ovmf(self):
        if not self.OVMF_FILE.parent.exists():
            self.OVMF_FILE.parent.mkdir(parents=True, exist_ok=True)

        if not self.OVMF_FILE.exists():
            print("[*] Скачивание OVMF в limine-tools/ovmf...")
            url = "https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd"
            run(["curl", "-Lo", str(self.OVMF_FILE), url])

    def create_iso(self):
        print("[*] Создание ISO...")

        boot_dir = self.ISO_DIR / "boot"
        limine_dir = boot_dir / "limine"
        efi_dir = self.ISO_DIR / "EFI" / "BOOT"

        # Создаём структуру каталогов, если их нет
        for d in [boot_dir, limine_dir, efi_dir]:
            d.mkdir(parents=True, exist_ok=True)

        # Копируем ядро (всегда обновляем)
        kernel_src = self.BUILD_DIR / self.OUTPUT
        run(["cp", str(kernel_src), str(boot_dir / "kernel")])

        # Копируем конфиг Limine, если его нет
        if not (limine_dir / "limine.conf").exists():
            run(["cp", "limine.conf", str(limine_dir)])

        # Копируем файлы Limine из limine-tools (всегда обновляем)
        files_to_copy = [
            ("limine-bios.sys", "boot/limine"),
            ("limine-bios-cd.bin", "boot/limine"),
            ("limine-uefi-cd.bin", "boot/limine"),
            ("BOOTX64.EFI", "EFI/BOOT"),
            ("BOOTIA32.EFI", "EFI/BOOT"),
        ]

        for src, dst in files_to_copy:
            run(["cp", str(self.LIMINE_DIR / src), str(self.ISO_DIR / dst)])

        # Создаём ISO
        xorriso_cmd = [
            "xorriso", "-as", "mkisofs", "-R", "-r", "-J",
            "-b", "boot/limine/limine-bios-cd.bin",
            "-no-emul-boot", "-boot-load-size", "4", "-boot-info-table",
            "-hfsplus", "-apm-block-size", "2048",
            "--efi-boot", "boot/limine/limine-uefi-cd.bin",
            "-efi-boot-part", "--efi-boot-image", "--protective-msdos-label",
            str(self.ISO_DIR), "-o", str(self.ISO_FILE)
        ]
        run(xorriso_cmd)

        # Устанавливаем Limine в образ
        run([str(self.LIMINE_DIR / "limine"), "bios-install", str(self.ISO_FILE)])
        print(f"[OK] ISO создан: {self.ISO_FILE}")

    def save_demo_iso(self):
        """Копирует ISO в demo_iso/ с именем NAME.VERSION-TIMESTAMP.iso"""
        dest = self.DEMO_ISO_DIR / self.FINAL_ISO_NAME
        run(["cp", str(self.ISO_FILE), str(dest)])
        print(f"[OK] ISO сохранён как: {dest}")

    def run_qemu_uefi(self, cleanup_after=False):
        if not self.ISO_FILE.exists():
            print(f"[!] ISO не найден: {self.ISO_FILE}")
            sys.exit(1)
        self.download_ovmf()
        cmd = (
            f"{self.QEMU} -M q35 "
            f"-drive if=pflash,unit=0,format=raw,file={self.OVMF_FILE},readonly=on "
            f"-cdrom {self.ISO_FILE} -m 2G "
            "-serial stdio" 
        )
        try:
            run(cmd, shell=True)
        finally:
            if cleanup_after:
                print("\n[CLEANUP] Закрытие QEMU — запуск очистки...")
                self.clean()

    def _is_text_file(self, path: Path) -> bool:
        """Проверяет, является ли файл текстовым (безопасно читаемым)."""
        try:
            with open(path, 'r', encoding='utf-8') as f:
                f.read(1024)
            return True
        except (UnicodeDecodeError, PermissionError, IsADirectoryError, FileNotFoundError):
            return False

    def generate_tree_with_content(self, output_file="OS-TREE.txt"):
        """Генерирует OS-TREE.txt с деревом проекта и содержимым текстовых файлов (с исключениями)."""
        print(f"[*] Генерация дерева системы в {output_file}...")

        with open(output_file, "w", encoding="utf-8") as out:
            root = Path(".")

            skip_dirs = {
                "bin-x86_64", "obj-x86_64",
                "cc-runtime", "freestnd-c-hdrs",
                "limine-protocol", "ovmf", "limine"
            }

            exclude_dirs = {"__pycache__", ".git", "iso_root"}
            exclude_files = {"*.o", "*.d", "*.fd", "*.iso", "*.hdd", "*.pyc"}

            def should_exclude(p: Path):
                if any(part.startswith('.') for part in p.parts):
                    return True
                if any(excl in str(p) for excl in exclude_dirs):
                    return True
                if any(p.match(pattern) for pattern in exclude_files):
                    return True
                return False

            def write_tree(path: Path, prefix="", is_last=True):
                connector = "└── " if is_last else "├── "
                out.write(f"{prefix}{connector}{path.name}\n")

                if path.is_dir():
                    if path.name in skip_dirs:
                        out.write(f"{prefix}    └── <скрыто: системная/бинарная директория>\n")
                        return
                    children = sorted([p for p in path.iterdir() if not should_exclude(p)])
                    for i, child in enumerate(children):
                        extension = "    " if is_last else "│   "
                        write_tree(child, prefix + extension, i == len(children) - 1)
                else:
                    if path.name == "build.py":
                        try:
                            out.write(f"{prefix}    │\n")
                            out.write(f"{prefix}    ├── CONTENT:\n")
                            with open(path, 'r', encoding='utf-8') as f:
                                content = f.read().strip()
                                lines = content.splitlines() or ["<empty>"]
                                for line in lines:
                                    out.write(f"{prefix}    │   {line}\n")
                            out.write(f"{prefix}    │\n")
                        except Exception as e:
                            out.write(f"{prefix}    │   <ошибка чтения: {e}>\n")
                        return

                    if self._is_text_file(path):
                        try:
                            out.write(f"{prefix}    │\n")
                            out.write(f"{prefix}    ├── CONTENT:\n")
                            with open(path, 'r', encoding='utf-8') as f:
                                content = f.read().strip()
                                lines = content.splitlines() or ["<empty>"]
                                for line in lines:
                                    out.write(f"{prefix}    │   {line}\n")
                            out.write(f"{prefix}    │\n")
                        except Exception as e:
                            out.write(f"{prefix}    │   <ошибка чтения: {e}>\n")

            out.write("📁 OS Project Tree (with text file contents)\n")
            out.write("=" * 80 + "\n")
            write_tree(root)

        print(f"[OK] Дерево с содержимым сохранено в {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description="Сборка и запуск DEER OS (x86_64)"
    )
    parser.add_argument("--run", action="store_true",
                        help="Автоматически собрать и запустить в QEMU (UEFI по умолчанию)")
    parser.add_argument("--clean", action="store_true",
                        help="Очистить объектные файлы (без последующей сборки)")
    parser.add_argument("--distclean", action="store_true",
                        help="Полная очистка: удалить всё, включая зависимости и образы")
    parser.add_argument("--tree", action="store_true",
                        help="Сгенерировать OS-TREE.txt с деревом и содержимым текстовых файлов")
    parser.add_argument("--name", type=str,
                        help="Имя ОС (например, MyDeer)")
    parser.add_argument("--cleangit", type=str,
                        help="Очистка проекта перед коммитом")
    parser.add_argument("--version", type=str,
                        help="Версия ОС (например, v0.1-alpha)")

    args = parser.parse_args()
    builder = Builder(name=args.name, version=args.version)

    if args.clean:
        builder.clean()
    elif args.cleangit:
        builder.gitclean()
    elif args.distclean:
        builder.distclean()
    elif args.tree:
        builder.generate_tree_with_content()
    elif args.run:
        builder.ensure_deps()
        builder.build_kernel()
        builder.clone_limine()
        builder.create_iso()
        builder.save_demo_iso()  # Сохраняем ISO с именем и временем
        builder.generate_tree_with_content()
        builder.run_qemu_uefi(cleanup_after=True)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()