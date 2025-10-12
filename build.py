#!/usr/bin/env python3
"""
OSDev Build System 

OSCRIPTUM

Универсальная система сборки для операционных систем с поддержкой:
- Кросс-компиляции x86_64
- Генерации ISO образов
- Тестирования в QEMU
- Управления версиями
- Кастомизации параметров ОС
"""

import os
import sys
import subprocess
from pathlib import Path
import argparse
import shutil
import time
import json
from datetime import datetime
from typing import List, Dict, Optional, Tuple
from enum import Enum, auto
import readline
import glob

# === 🔧 Константы: имена ключевых файлов (базовые, БЕЗ жёсткого имени ОС) ===
KERNEL_SRC = 'kernel.c'
BOOT_ASM = 'boot.S'
LINKER_SCRIPT = 'link.ld'
GRUB_CONFIG = 'grub.cfg'


class LogLevel(Enum):
    DEBUG = auto()
    INFO = auto()
    WARNING = auto()
    ERROR = auto()
    CRITICAL = auto()


class Color:
    PURPLE = '\033[95m'
    CYAN = '\033[96m'
    DARKCYAN = '\033[36m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'


class OSConfig:
    """Конфигурация операционной системы"""
    def __init__(self):
        # Основные настройки ОС
        self.NAME = "OS"
        self.VERSION = "0.0.1"
        self.AUTHOR = "Anonymous OSDev"
        self.DESCRIPTION = "A simple 64-bit OS that prints 'ABC 64 BIT'"
        self.BUILD_DATE = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # --- Динамические имена файлов ---
        self.ELF_KERNEL = f'{self.NAME.lower()}.elf'
        self.BIN_KERNEL = f'{self.NAME.lower()}.bin'
        self.ISO_OUTPUT = f'{self.NAME.lower()}.iso'

        # Настройки инструментов
        self.ARCH = 'x86_64'
        self.TARGET = 'x86_64-elf'
        self.CC = f'{self.TARGET}-gcc'
        self.LD = f'{self.TARGET}-ld'
        self.OBJCOPY = f'{self.TARGET}-objcopy'
        self.GRUB_FILE = 'grub-file'
        self.GRUB_MKRESCUE = 'grub-mkrescue'
        
        # Флаги компиляции
        self.CFLAGS = '-ffreestanding -O2 -Wall -Wextra -std=gnu11 -mno-red-zone -fno-stack-protector -fno-pic'
        self.LDFLAGS = '-nostdlib'
        self.INCLUDES = ['include']
        
        # Директории с исходным кодом
        self.SOURCE_DIRS = ['src']
        
        # Пути для выходных файлов
        self.BUILD_DIR = 'build'
        self.BIN_DIR = 'bin'
        self.DEMO_ISO_DIR = 'demo_iso'
        self.ISO_DIR = 'isodir'
        self.GRUB_CFG = f'src/{GRUB_CONFIG}'
        self.TREE_FILE = 'OS-TREE.txt'

        # Настройки QEMU
        self.QEMU_CMD = 'qemu-system-x86_64'
        self.QEMU_MEMORY = '512M'
        self.QEMU_AUDIO = False
        self.QEMU_SERIAL = True
        self.QEMU_CLEAN_AFTER_RUN = False
        self.QEMU_GDB_PORT = 1234

        # Настройки GRUB
        self.GRUB_TIMEOUT = 10
        self.GRUB_DEFAULT = 0
        self.GRUB_MENU_COLORS = {
            'normal': 'white/black',
            'highlight': 'black/light-gray'
        }

    def get_kernel_output(self) -> str:
        return f'{self.BIN_DIR}/{self.ELF_KERNEL}'

    def get_kernel_binary(self) -> str:
        return f'{self.BIN_DIR}/{self.BIN_KERNEL}'

    def get_iso_image(self, versioned: bool = False) -> str:
        if versioned:
            timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
            return f'{self.DEMO_ISO_DIR}/{self.NAME.lower()}-v{self.VERSION}-{timestamp}.iso'
        return f'{self.BIN_DIR}/{self.ISO_OUTPUT}'

    def load_config(self, config_file: str = 'os_config.json') -> None:
        if not os.path.exists(config_file):
            self.create_default_config(config_file)
        try:
            with open(config_file, 'r') as f:
                config_data = json.load(f)
                for key, value in config_data.items():
                    if hasattr(self, key):
                        if key in ['QEMU_AUDIO', 'QEMU_SERIAL', 'QEMU_CLEAN_AFTER_RUN']:
                            value = str(value).lower() in ('true', '1', 't', 'y', 'yes')
                        setattr(self, key, value)
                self._update_dynamic_names()
            self.log(f"Конфигурация загружена из {config_file}", LogLevel.INFO)
        except Exception as e:
            self.log(f"Ошибка загрузки конфигурации: {e}", LogLevel.ERROR)

    def save_config(self, config_file: str = 'os_config.json') -> None:
        config_data = {
            'NAME': self.NAME,
            'VERSION': self.VERSION,
            'AUTHOR': self.AUTHOR,
            'DESCRIPTION': self.DESCRIPTION,
            'ARCH': self.ARCH,
            'TARGET': self.TARGET,
            'CFLAGS': self.CFLAGS,
            'LDFLAGS': self.LDFLAGS,
            'QEMU_MEMORY': self.QEMU_MEMORY,
            'QEMU_AUDIO': self.QEMU_AUDIO,
            'QEMU_SERIAL': self.QEMU_SERIAL,
            'QEMU_CLEAN_AFTER_RUN': self.QEMU_CLEAN_AFTER_RUN,
            'GRUB_TIMEOUT': self.GRUB_TIMEOUT,
            'GRUB_DEFAULT': self.GRUB_DEFAULT,
            'GRUB_MENU_COLORS': self.GRUB_MENU_COLORS
        }
        try:
            with open(config_file, 'w') as f:
                json.dump(config_data, f, indent=4)
            self.log(f"Конфигурация сохранена в {config_file}", LogLevel.INFO)
        except Exception as e:
            self.log(f"Ошибка сохранения конфигурации: {e}", LogLevel.ERROR)

    def create_default_config(self, config_file: str = 'os_config.json') -> None:
        default_config = {
            'NAME': 'OS',
            'VERSION': '0.0.1',
            'AUTHOR': 'Anonymous OSDev',
            'DESCRIPTION': 'A simple 64-bit OS that prints \'ABC 64 BIT\'',
            'ARCH': 'x86_64',
            'TARGET': 'x86_64-elf',
            'CFLAGS': '-ffreestanding -O2 -Wall -Wextra -std=gnu11 -mno-red-zone -fno-stack-protector -fno-pic',
            'LDFLAGS': '-nostdlib',
            'QEMU_MEMORY': '512M',
            'QEMU_AUDIO': False,
            'QEMU_SERIAL': True,
            'QEMU_CLEAN_AFTER_RUN': False,
            'GRUB_TIMEOUT': 0,
            'GRUB_DEFAULT': 0,
            'GRUB_MENU_COLORS': {
                'normal': 'white/black',
                'highlight': 'black/light-gray'
            }
        }
        try:
            with open(config_file, 'w') as f:
                json.dump(default_config, f, indent=4)
            self.log(f"Создан стандартный конфиг: {config_file}", LogLevel.INFO)
        except Exception as e:
            self.log(f"Ошибка создания os_config.json: {e}", LogLevel.ERROR)

    def _update_dynamic_names(self):
        """Обновить имена файлов при изменении NAME"""
        self.ELF_KERNEL = f'{self.NAME.lower()}.elf'
        self.BIN_KERNEL = f'{self.NAME.lower()}.bin'
        self.ISO_OUTPUT = f'{self.NAME.lower()}.iso'

    def edit_config_interactive(self) -> None:
        print(f"{Color.BOLD}{Color.CYAN}Редактирование конфигурации ОС{Color.END}")
        print(f"{Color.YELLOW}Оставьте поле пустым, чтобы сохранить текущее значение{Color.END}")

        config_items = [
            ('NAME', 'Имя ОС', str),
            ('VERSION', 'Версия ОС', str),
            ('AUTHOR', 'Автор', str),
            ('DESCRIPTION', 'Описание', str),
            ('ARCH', 'Архитектура', str),
            ('TARGET', 'Целевая платформа', str),
            ('QEMU_MEMORY', 'Память QEMU (например, 512M)', str),
            ('QEMU_AUDIO', 'Звук в QEMU (y/n)', lambda x: x.lower() in ('y', 'yes')),
            ('QEMU_SERIAL', 'Последовательный порт в QEMU (y/n)', lambda x: x.lower() in ('y', 'yes')),
            ('QEMU_CLEAN_AFTER_RUN', 'Очистка после запуска QEMU (y/n)', lambda x: x.lower() in ('y', 'yes')),
            ('GRUB_TIMEOUT', 'Таймаут меню GRUB (сек, -1 = нет меню)', int),
            ('GRUB_DEFAULT', 'Пункт по умолчанию в меню GRUB', int),
        ]

        for attr, prompt, validator in config_items:
            current_value = getattr(self, attr)
            while True:
                try:
                    user_input = input(f"{prompt} [{current_value}]: ").strip()
                    if not user_input:
                        break
                    new_value = validator(user_input) if validator != bool else (user_input.lower() in ('y', 'yes'))
                    setattr(self, attr, new_value)
                    break
                except ValueError:
                    print(f"{Color.RED}Некорректное значение!{Color.END}")

        self._update_dynamic_names()
        self.save_config()
        print(f"{Color.GREEN}Конфигурация успешно обновлена!{Color.END}")

    def log(self, message: str, level: LogLevel = LogLevel.INFO) -> None:
        colors = {
            LogLevel.DEBUG: Color.DARKCYAN,
            LogLevel.INFO: Color.GREEN,
            LogLevel.WARNING: Color.YELLOW,
            LogLevel.ERROR: Color.RED,
            LogLevel.CRITICAL: Color.RED + Color.BOLD
        }
        prefix = {
            LogLevel.DEBUG: "[DEBUG]",
            LogLevel.INFO: "[INFO]",
            LogLevel.WARNING: "[WARNING]",
            LogLevel.ERROR: "[ERROR]",
            LogLevel.CRITICAL: "[CRITICAL]"
        }
        print(f"{colors.get(level, '')}{prefix.get(level, '')} {message}{Color.END}")


class BuildSystem:
    """Система сборки операционной системы"""
    def __init__(self, config: OSConfig):
        self.config = config
        self.start_time = time.time()

    def print_banner(self) -> None:
        banner = fr"""
{Color.PURPLE}{Color.BOLD}
   ___  ____   ____ ____  ___ ____ _____ _   _ __  __ 
  / _ \/ ___| / ___|  _ \|_ _|  _ \_   _| | | |  \/  |
 | | | \___ \| |   | |_) || || |_) || | | | | | |\/| |
 | |_| |___) | |___|  _ < | ||  __/ | | | |_| | |  | |
  \___/|____/ \____|_| \_\___|_|    |_|  \___/|_|  |_|
                                                      
{Color.END}
{Color.CYAN}OS Name:    {Color.BOLD}{self.config.NAME}{Color.END}
{Color.CYAN}Version:    {Color.BOLD}{self.config.VERSION}{Color.END}
{Color.CYAN}Author:     {Color.BOLD}{self.config.AUTHOR}{Color.END}
{Color.CYAN}Description:{Color.BOLD}{self.config.DESCRIPTION}{Color.END}
{Color.CYAN}Build Date: {Color.BOLD}{self.config.BUILD_DATE}{Color.END}
{Color.CYAN}Target:     {Color.BOLD}{self.config.TARGET}{Color.END}
{Color.CYAN}Arch:       {Color.BOLD}{self.config.ARCH}{Color.END}
"""
        print(banner)

    def ensure_dir(self, path: str) -> None:
        Path(path).mkdir(parents=True, exist_ok=True)
        self.config.log(f"Директория {path} создана/проверена", LogLevel.DEBUG)

    def find_sources(self, extensions: Tuple[str, ...] = ('.c', '.S', '.asm')) -> List[str]:
        sources = []
        exclude_files = {BOOT_ASM}

        for directory in self.config.SOURCE_DIRS:
            for root, _, files in os.walk(directory):
                for file in files:
                    if file.endswith(extensions) and file not in exclude_files:
                        full_path = os.path.join(root, file)
                        sources.append(full_path)
                        self.config.log(f"Добавлен источник: {full_path}", LogLevel.DEBUG)

        self.config.log(f"Найдено {len(sources)} исходных файлов", LogLevel.INFO)
        return sources

    def compile_sources(self, sources: List[str], obj_dir: str) -> List[str]:
        objects = []
        include_flags = ' '.join(f'-I{inc}' for inc in self.config.INCLUDES)

        for src in sources:
            obj = os.path.join(obj_dir, f"{Path(src).stem}.o")
            objects.append(obj)

            if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src):
                continue

            cmd = f"{self.config.CC} {self.config.CFLAGS} {include_flags} -c {src} -o {obj}"
            self.config.log(f"Компиляция: {src}", LogLevel.INFO)
            try:
                subprocess.run(cmd, shell=True, check=True)
            except subprocess.CalledProcessError as e:
                self.config.log(f"Ошибка компиляции {src}: {e}", LogLevel.ERROR)
                sys.exit(1)

        return objects

    def link_kernel(self, objects: List[str], output: str, linker_script: str) -> None:
        cmd = (
            f"{self.config.CC} {self.config.LDFLAGS} -T {linker_script} "
            f"-o {output} {' '.join(objects)}"
        )
        self.config.log(f"Линковка ядра -> {output}", LogLevel.INFO)
        try:
            subprocess.run(cmd, shell=True, check=True)
        except subprocess.CalledProcessError as e:
            self.config.log(f"Ошибка линковки: {e}", LogLevel.ERROR)
            sys.exit(1)

        self.config.log("Проверка Multiboot2 совместимости", LogLevel.INFO)
        try:
            result = subprocess.run(
                [self.config.GRUB_FILE, '--is-x86-multiboot2', output],
                capture_output=True
            )
            if result.returncode != 0:
                self.config.log("Файл не является Multiboot2 совместимым!", LogLevel.WARNING)
        except Exception:
            pass

    def create_kernel_binary(self, elf_kernel: str, bin_kernel: str) -> None:
        if not os.path.exists(elf_kernel):
            self.config.log(f"ELF файл ядра не найден: {elf_kernel}", LogLevel.ERROR)
            return

        if os.path.exists(bin_kernel) and os.path.getmtime(bin_kernel) >= os.path.getmtime(elf_kernel):
            self.config.log(f"Бинарный файл актуален: {bin_kernel}", LogLevel.DEBUG)
            return

        self.config.log(f"Создание бинарного файла: {bin_kernel}", LogLevel.INFO)
        cmd = f"{self.config.OBJCOPY} -O binary {elf_kernel} {bin_kernel}"
        try:
            subprocess.run(cmd, shell=True, check=True)
            self.config.log(f"Бинарный файл создан: {bin_kernel}", LogLevel.INFO)
        except subprocess.CalledProcessError as e:
            self.config.log(f"Ошибка создания .bin: {e}", LogLevel.ERROR)
            sys.exit(1)
            
    def create_grub_cfg(self, grub_cfg_path: str) -> None:
        """
        Создаёт стандартный grub.cfg, ТОЛЬКО если он не существует.
        Это позволяет пользователю редактировать свой собственный конфиг.
        """
        if os.path.exists(grub_cfg_path):
            self.config.log(f"Используется существующий GRUB конфиг: {grub_cfg_path}", LogLevel.INFO)
            return

        # Если файла нет — создаём шаблон по умолчанию
        grub_content = f"""
    # === GRUB CONFIG FOR {self.config.NAME.upper()} ===
    # Этот файл был создан автоматически.
    # Теперь вы можете отредактировать его вручную.
    # Следующие сборки будут использовать эту версию.

    timeout={self.config.GRUB_TIMEOUT}
    default={self.config.GRUB_DEFAULT}

    menuentry "{self.config.NAME} OS v{self.config.VERSION}" {{
        multiboot2 /boot/{self.config.ELF_KERNEL}
        boot
    }}
    """

        try:
            self.ensure_dir(os.path.dirname(grub_cfg_path))
            with open(grub_cfg_path, 'w') as f:
                f.write(grub_content.strip() + '\n')
            self.config.log(f"✅ Создан стандартный GRUB конфиг: {grub_cfg_path}", LogLevel.INFO)
            self.config.log(f"💡 Теперь вы можете отредактировать его вручную!", LogLevel.INFO)
        except Exception as e:
            self.config.log(f"Ошибка создания grub.cfg: {e}", LogLevel.ERROR)
            sys.exit(1)

    def create_iso(self, kernel_bin: str, iso_dir: str, grub_cfg_src: str, iso_image: str) -> None:
        boot_dir = f"{iso_dir}/boot"
        grub_dir = f"{boot_dir}/grub"

        self.ensure_dir(boot_dir)
        self.ensure_dir(grub_dir)

        # Копируем .efi (ELF) вместо .bin
        kernel_elf = self.config.get_kernel_output()
        if not os.path.exists(kernel_elf):
            self.config.log(f"Фатально: ELF ядро не найдено: {kernel_elf}", LogLevel.CRITICAL)
            sys.exit(1)

        shutil.copy(kernel_elf, f"{boot_dir}/{self.config.ELF_KERNEL}")  # deer.efi

        # Копируем grub.cfg
        shutil.copy(grub_cfg_src, f"{grub_dir}/{GRUB_CONFIG}")

        self.config.log(f"Создание ISO: {iso_image}", LogLevel.INFO)
        try:
            subprocess.run(
                f"{self.config.GRUB_MKRESCUE} -o {iso_image} {iso_dir}",
                shell=True, check=True
            )
        except subprocess.CalledProcessError as e:
            self.config.log(f"Ошибка создания ISO: {e}", LogLevel.ERROR)
            sys.exit(1)

    def run_qemu(self, iso_image: str, clean_after: bool = False, gdb: bool = False) -> None:
        if not os.path.exists(iso_image):
            demo_iso_pattern = f"{self.config.DEMO_ISO_DIR}/{self.config.NAME.lower()}-v*"
            demo_isos = sorted(glob.glob(demo_iso_pattern), reverse=True)
            if demo_isos:
                iso_image = demo_isos[0]
                self.config.log(f"Используется последний ISO: {iso_image}", LogLevel.INFO)
            else:
                self.config.log("ISO не найден", LogLevel.ERROR)
                return

        qemu_cmd = [
            self.config.QEMU_CMD,
            '-m', self.config.QEMU_MEMORY,
            '-cdrom', iso_image,
            '-d', 'guest_errors',
            '-no-reboot',
            '-name', f"{self.config.NAME} v{self.config.VERSION}"
        ]

        if gdb:
            qemu_cmd += ['-s', '-S']
            self.config.log(f"QEMU запущен в режиме отладки: target remote localhost:{self.config.QEMU_GDB_PORT}", LogLevel.INFO)

        if self.config.QEMU_SERIAL:
            qemu_cmd += ['-serial', 'stdio']

        if self.config.QEMU_AUDIO:
            qemu_cmd += ['-audiodev', 'pa,id=audio0', '-machine', 'pcspk-audiodev=audio0']

        self.config.log("Запуск QEMU...", LogLevel.INFO)
        try:
            subprocess.run(qemu_cmd, check=True)
        except subprocess.CalledProcessError as e:
            self.config.log(f"QEMU завершился с ошибкой: {e}", LogLevel.ERROR)
        except KeyboardInterrupt:
            self.config.log("QEMU остановлен пользователем", LogLevel.INFO)
        finally:
            if clean_after:
                self.clean()

    def generate_file_tree(self, path: str = '.', indent: str = '', last: bool = True, output_file=None) -> None:
        prefix = indent + ('└── ' if last else '├── ')
        name = os.path.basename(path)

        if os.path.isdir(path):
            output_file.write(f"{prefix}[ПАПКА {name.upper()}]\n")
        else:
            output_file.write(f"{prefix}[ФАЙЛ {name.upper()}]\n")

        if os.path.isdir(path):
            items = sorted(os.listdir(path))
            for i, item in enumerate(items):
                is_last = i == len(items) - 1
                new_indent = indent + ('    ' if last else '│   ')
                self.generate_file_tree(os.path.join(path, item), new_indent, is_last, output_file)

                file_path = os.path.join(path, item)
                if os.path.isfile(file_path) and item.endswith(('.c', '.h', '.ld', '.S', '.txt')):
                    try:
                        with open(file_path, 'r', encoding='utf-8') as f:
                            content = f.read().strip()
                            if content:
                                output_file.write(f"{new_indent}    [СОДЕРЖИМОЕ]\n")
                                for line in content.split('\n'):
                                    output_file.write(f"{new_indent}    {line}\n")
                    except Exception as e:
                        output_file.write(f"{new_indent}    [ОШИБКА ЧТЕНИЯ ФАЙЛА: {str(e)}]\n")

    def save_file_tree(self) -> None:
        tree_file = self.config.TREE_FILE
        self.config.log(f"Сохранение дерева файлов в {tree_file}", LogLevel.INFO)
        with open(tree_file, 'w', encoding='utf-8') as f:
            f.write(f"Дерево файлов ОС {self.config.NAME} v{self.config.VERSION}\n")
            f.write(f"Сгенерировано: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 50 + "\n\n")
            self.generate_file_tree(output_file=f)
        self.config.log(f"Дерево файлов успешно сохранено в {tree_file}", LogLevel.INFO)

    def print_file_tree(self) -> None:
        self.save_file_tree()
        with open(self.config.TREE_FILE, 'r', encoding='utf-8') as f:
            print(f.read())

    def clean(self) -> None:
        dirs_to_clean = [self.config.BUILD_DIR, self.config.ISO_DIR]
        files_to_clean = [
            self.config.get_iso_image(),
            self.config.get_kernel_output(),
            self.config.get_kernel_binary()
        ]
        
        self.config.log("Очистка артефактов сборки", LogLevel.INFO)
        for directory in dirs_to_clean:
            if os.path.exists(directory):
                shutil.rmtree(directory)
                self.config.log(f"Удалена директория: {directory}", LogLevel.DEBUG)
        
        for file_pattern in files_to_clean:
            if os.path.exists(file_pattern):
                os.remove(file_pattern)
                self.config.log(f"Удален файл: {file_pattern}", LogLevel.DEBUG)

    def check_tools(self) -> None:
        required_tools = [
            self.config.CC,
            self.config.LD,
            self.config.OBJCOPY,
            self.config.GRUB_FILE,
            self.config.GRUB_MKRESCUE,
            self.config.QEMU_CMD
        ]
        
        self.config.log("Проверка инструментов сборки", LogLevel.INFO)
        for tool in required_tools:
            try:
                subprocess.run(f"which {tool}", shell=True, check=True, 
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except subprocess.CalledProcessError:
                self.config.log(f"Не найден необходимый инструмент: {tool}", LogLevel.ERROR)
                sys.exit(1)

    def build(self, debug: bool = False, versioned_iso: bool = False) -> None:
        self.print_banner()
        
        if debug:
            self.config.CFLAGS += ' -g -DDEBUG'
            self.config.log("Режим отладки включен", LogLevel.INFO)
        
        self.check_tools()
        
        self.ensure_dir(self.config.BUILD_DIR)
        self.ensure_dir(self.config.BIN_DIR)
        self.ensure_dir(self.config.ISO_DIR)
        self.ensure_dir(self.config.DEMO_ISO_DIR)
        
        try:
            if not os.path.exists('os_config.json'):
                self.config.create_default_config('os_config.json')
            
            self.create_grub_cfg(self.config.GRUB_CFG)

            sources = self.find_sources()
            objects = self.compile_sources(sources, self.config.BUILD_DIR)
            
            # === Компиляция ОДНОГО boot.s (с встроенным mb2) ===
            boot_src = f"src/kernel/{BOOT_ASM}"
            obj = os.path.join(self.config.BUILD_DIR, "boot.o")

            if not os.path.exists(boot_src):
                self.config.log(f"Фатально: не найден файл {boot_src}", LogLevel.CRITICAL)
                sys.exit(1)

            cmd = (
                f"{self.config.CC} {self.config.CFLAGS} "
                f"-x assembler-with-cpp -c {boot_src} -o {obj}"
            )
            self.config.log(f"Компиляция: {boot_src}", LogLevel.INFO)
            try:
                subprocess.run(cmd, shell=True, check=True)
                objects.insert(0, obj)  # Должен быть первым!
            except subprocess.CalledProcessError as e:
                self.config.log(f"Ошибка компиляции boot.s: {e}", LogLevel.ERROR)
                sys.exit(1)

            elf_kernel_path = self.config.get_kernel_output()
            self.link_kernel(
                objects=objects,
                output=elf_kernel_path,
                linker_script=f"src/kernel/{LINKER_SCRIPT}"
            )

            bin_kernel_path = self.config.get_kernel_binary()
            self.create_kernel_binary(elf_kernel_path, bin_kernel_path)

            iso_path = self.config.get_iso_image(versioned=versioned_iso)
            
            self.create_iso(
                kernel_bin=bin_kernel_path,
                iso_dir=self.config.ISO_DIR,
                grub_cfg_src=self.config.GRUB_CFG,
                iso_image=iso_path
            )
            
            if versioned_iso:
                self.config.log(f"ISO создан: {iso_path}", LogLevel.INFO)
            else:
                demo_iso = self.config.get_iso_image(versioned=True)
                shutil.copy(iso_path, demo_iso)
                self.config.log(f"ISO сохранен как: {demo_iso}", LogLevel.INFO)
            
            self.save_file_tree()
            
            build_time = time.time() - self.start_time
            self.config.log(f"Сборка завершена за {build_time:.2f} секунд", LogLevel.INFO)
            self.config.log(f"Файлы ядра: {elf_kernel_path}, {bin_kernel_path}", LogLevel.INFO)
            
        except KeyboardInterrupt:
            self.config.log("Сборка прервана пользователем", LogLevel.ERROR)
            sys.exit(1)


def main() -> None:
    config = OSConfig()
    config.load_config()
    config._update_dynamic_names()

    parser = argparse.ArgumentParser(
        description=f'{config.NAME} OS Build System',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument('--clean', action='store_true', help='Очистить артефакты сборки')
    parser.add_argument('--run', action='store_true', help='Собрать и запустить в QEMU')
    parser.add_argument('--debug', action='store_true', help='Сборка с отладочными символами')
    parser.add_argument('--demo', action='store_true', 
                       help='Создать версионный ISO в папке demo_iso')
    parser.add_argument('--name', type=str, help='Установить имя ОС')
    parser.add_argument('--version', type=str, help='Установить версию ОС')
    parser.add_argument('--author', type=str, help='Установить автора ОС')
    parser.add_argument('--description', type=str, help='Установить описание ОС')
    parser.add_argument('--save-config', action='store_true', 
                       help='Сохранить текущую конфигурацию')
    parser.add_argument('--edit-config', action='store_true', 
                       help='Интерактивное редактирование конфигурации')
    parser.add_argument('--clean-after-run', action='store_true', 
                       help='Очистить артефакты после запуска QEMU')
    parser.add_argument('--gdb', action='store_true', 
                       help='Запустить QEMU в режиме отладки GDB')
    parser.add_argument('--tree', action='store_true', 
                       help='Показать дерево файлов и папок ОС и сохранить в файл')
    parser.add_argument('--grub-timeout', type=int, help='Таймаут меню GRUB в секундах')
    parser.add_argument('--grub-default', type=int, help='Номер пункта меню по умолчанию')
    parser.add_argument('--no-grub-menu', action='store_true', help='Отключить меню GRUB (мгновенный запуск)')
    
    args = parser.parse_args()
    
    if args.edit_config:
        config.edit_config_interactive()
        return
    
    updated = False

    if args.name:
        config.NAME = args.name
        config._update_dynamic_names()
        config.save_config()
        config.log(f"✅ Имя ОС установлено как '{config.NAME}'", LogLevel.INFO)
        updated = True

    if args.version:
        config.VERSION = args.version
        config.save_config()
        config.log(f"✅ Версия установлена: {config.VERSION}", LogLevel.INFO)
        updated = True

    if args.author:
        config.AUTHOR = args.author
        config.save_config()
        updated = True

    if args.description:
        config.DESCRIPTION = args.description
        config.save_config()
        updated = True

    if args.grub_timeout is not None:
        config.GRUB_TIMEOUT = args.grub_timeout
        config.save_config()
        config.log(f"✅ Таймаут GRUB установлен: {config.GRUB_TIMEOUT} сек", LogLevel.INFO)
        updated = True

    if args.no_grub_menu:
        config.GRUB_TIMEOUT = -1
        config.save_config()
        config.log("✅ Меню GRUB отключено (мгновенный запуск)", LogLevel.INFO)
        updated = True

    if args.grub_default is not None:
        config.GRUB_DEFAULT = args.grub_default
        config.save_config()
        config.log(f"✅ Пункт меню GRUB по умолчанию: {config.GRUB_DEFAULT}", LogLevel.INFO)
        updated = True

    if args.clean_after_run:
        config.QEMU_CLEAN_AFTER_RUN = True
        config.save_config()

    if args.save_config or updated:
        config.save_config()
        return
    
    builder = BuildSystem(config)
    
    if args.tree:
        builder.print_file_tree()
        return
    
    if args.clean:
        builder.clean()
        return
    
    builder.build(
        debug=args.debug,
        versioned_iso=args.demo or args.run
    )
    
    if args.run:
        iso_path = config.get_iso_image(versioned=True)
        if not os.path.exists(iso_path):
            iso_path = config.get_iso_image()
        
        builder.run_qemu(
            iso_path,
            clean_after=args.clean_after_run or config.QEMU_CLEAN_AFTER_RUN,
            gdb=args.gdb
        )


if __name__ == '__main__':
    main()