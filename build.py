#!/usr/bin/env python3

import os
import sys
import subprocess
import argparse
from pathlib import Path


def run(cmd, check=True, shell=False, cwd=None):
    """Универсальная обёртка для выполнения команд."""
    print(f"[RUN] {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    result = subprocess.run(cmd, shell=shell, check=check, cwd=cwd)
    return result


class Builder:
    def __init__(self):
        self.ARCH = "x86_64"
        self.OUTPUT = "kernel"
        self.IMAGE_NAME = f"deer-v0.0.1"
        self.KERNEL_DIR = Path("kernel")
        self.BUILD_DIR = self.KERNEL_DIR / f"bin-{self.ARCH}"
        self.OBJ_DIR = self.KERNEL_DIR / f"obj-{self.ARCH}"
        self.LINKER_SCRIPT = self.KERNEL_DIR / "linker-scripts" / f"{self.ARCH}.lds"

        self.LIMINE_DIR = Path("limine")
        self.OVMF_DIR = Path("ovmf")
        self.OVMF_FILE = self.OVMF_DIR / f"ovmf-code-{self.ARCH}.fd"

        self.ISO_DIR = Path("iso_root")
        self.ISO_FILE = Path(f"{self.IMAGE_NAME}.iso")
        self.HDD_FILE = Path(f"{self.IMAGE_NAME}.hdd")

        self.QEMU = f"qemu-system-{self.ARCH}"

    def clean(self):
        """Очистить временные файлы сборки и ISO."""
        cleaned = []
        if self.BUILD_DIR.exists():
            run(["rm", "-rf", str(self.BUILD_DIR)])
            cleaned.append(str(self.BUILD_DIR))
        if self.OBJ_DIR.exists():
            run(["rm", "-rf", str(self.OBJ_DIR)])
            cleaned.append(str(self.OBJ_DIR))
        if self.ISO_DIR.exists():
            run(["rm", "-rf", str(self.ISO_DIR)])
            cleaned.append(str(self.ISO_DIR))
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
        if self.LIMINE_DIR.exists():
            run(["rm", "-rf", str(self.LIMINE_DIR)])
        if self.OVMF_DIR.exists():
            run(["rm", "-rf", str(self.OVMF_DIR)])
        deps_flag = self.KERNEL_DIR / ".deps-obtained"
        if deps_flag.exists():
            deps_flag.unlink()
        print("[OK] Полная очистка завершена.")

    def ensure_deps(self):
        deps_flag = self.KERNEL_DIR / ".deps-obtained"
        if not deps_flag.exists():
            print("[!] Зависимости не установлены. Запускаем get-deps...")
            run(["./get-deps"], cwd=str(self.KERNEL_DIR))
            deps_flag.touch()

    def build_kernel(self):

        print("[*] Сборка ядра для x86_64...")

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

        CFLAGS = [
            "-g", "-O2", "-pipe", "-Wall", "-Wextra", "-std=gnu11",
            "-nostdinc", "-ffreestanding", "-fno-stack-protector",
            "-fno-stack-check", "-fno-lto", "-fno-PIC",
            "-ffunction-sections", "-fdata-sections",
            "-m64", "-march=x86-64", "-mabi=sysv",
            "-mno-80387", "-mno-mmx", "-mno-sse", "-mno-sse2",
            "-mno-red-zone", "-mcmodel=kernel"
        ]

        CPPFLAGS = [
            f"-I{self.KERNEL_DIR}/src",
            f"-I{self.KERNEL_DIR}/limine-protocol/include",
            f"-isystem{self.KERNEL_DIR}/freestnd-c-hdrs/include",
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

        # Компиляция .c
        for src in all_c:
            rel_src = Path("src") / src
            obj = self.OBJ_DIR / rel_src.with_suffix(".c.o")
            os.makedirs(obj.parent, exist_ok=True)
            cmd = [CC] + CFLAGS + CPPFLAGS + ["-c", str(self.KERNEL_DIR / src), "-o", str(obj)]
            run(cmd)
            objects.append(obj)

        # Компиляция .S
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

        # Линковка
        kernel_out = self.BUILD_DIR / self.OUTPUT
        os.makedirs(self.BUILD_DIR, exist_ok=True)
        link_cmd = [LD] + LDFLAGS + [str(obj) for obj in objects] + ["-o", str(kernel_out)]
        run(link_cmd)
        print(f"[OK] Ядро собрано: {kernel_out}")

    def clone_limine(self):
        if not self.LIMINE_DIR.exists():
            print("[*] Клонирование Limine binary release...")
            run([
                "git", "clone",
                "--branch", "v10.x-binary",
                "--depth", "1",
                "https://codeberg.org/Limine/Limine.git",
                "limine"
            ])
        run(["make"], cwd=self.LIMINE_DIR)

    def download_ovmf(self):
        if not self.OVMF_FILE.exists():
            print("[*] Скачивание OVMF...")
            self.OVMF_DIR.mkdir(exist_ok=True)
            url = f"https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd"
            run(["curl", "-Lo", str(self.OVMF_FILE), url])

    def create_iso(self):
        if self.ISO_FILE.exists():
            print(f"[*] ISO уже существует — пропускаем создание.")
            return

        print("[*] Создание ISO...")

        if self.ISO_DIR.exists():
            run(["rm", "-rf", str(self.ISO_DIR)])

        boot_dir = self.ISO_DIR / "boot"
        limine_dir = boot_dir / "limine"
        efi_dir = self.ISO_DIR / "EFI" / "BOOT"

        for d in [boot_dir, limine_dir, efi_dir]:
            d.mkdir(parents=True, exist_ok=True)

        kernel_src = self.BUILD_DIR / self.OUTPUT
        run(["cp", str(kernel_src), str(boot_dir / "kernel")])
        run(["cp", "limine.conf", str(limine_dir)])

        # Копируем файлы Limine
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

    def run_qemu_uefi(self, cleanup_after=False):
        if not self.ISO_FILE.exists():
            print(f"[!] ISO не найден: {self.ISO_FILE}")
            sys.exit(1)
        self.download_ovmf()
        cmd = (
            f"{self.QEMU} -M q35 "
            f"-drive if=pflash,unit=0,format=raw,file={self.OVMF_FILE},readonly=on "
            f"-cdrom {self.ISO_FILE} -m 2G"
        )
        try:
            run(cmd, shell=True)
        finally:
            if cleanup_after:
                print("\n[CLEANUP] Закрытие QEMU — запуск очистки...")
                self.clean()


def main():
    parser = argparse.ArgumentParser(
        description="Сборка и запуск DEER OS (x86_64)"
    )
    parser.add_argument(
        "--run", action="store_true",
        help="Автоматически собрать и запустить в QEMU (UEFI по умолчанию)"
    )
    parser.add_argument(
        "--clean", action="store_true",
        help="Очистить объектные файлы (без последующей сборки)"
    )
    parser.add_argument(
        "--distclean", action="store_true",
        help="Полная очистка: удалить всё, включая зависимости и образы"
    )
    parser.add_argument(
        "--clean-after-run", action="store_true",
        help="Очистить временные файлы после закрытия QEMU"
    )

    args = parser.parse_args()

    builder = Builder()

    if args.clean:
        builder.clean()
        return
    if args.distclean:
        builder.distclean()
        return
    if args.run:
        builder.ensure_deps()
        builder.build_kernel()
        builder.clone_limine()
        builder.create_iso()
        builder.run_qemu_uefi(cleanup_after=args.clean_after_run)
        return

    parser.print_help()


if __name__ == "__main__":
    main()