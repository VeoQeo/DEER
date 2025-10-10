#!/usr/bin/env python3
"""
OS Development Environment Setup Script

Расширенный скрипт для установки инструментов разработки операционных систем.
Поддерживает установку кросс-компиляторов x86_64-elf-gcc и других инструментов OSDev.
"""

import os
import sys
import subprocess
import argparse
import platform
import shutil
from pathlib import Path
import tempfile

class Colors:
    """Цвета для красивого вывода"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

def print_header(text):
    """Выводит заголовок"""
    print(f"\n{Colors.CYAN}{Colors.BOLD}{'='*80}{Colors.END}")
    print(f"{Colors.CYAN}{Colors.BOLD}{text:^80}{Colors.END}")
    print(f"{Colors.CYAN}{Colors.BOLD}{'='*80}{Colors.END}")

def print_success(text):
    """Выводит успешное сообщение"""
    print(f"{Colors.GREEN}✓ {text}{Colors.END}")

def print_error(text):
    """Выводит сообщение об ошибке"""
    print(f"{Colors.RED}✗ {text}{Colors.END}")

def print_warning(text):
    """Выводит предупреждение"""
    print(f"{Colors.YELLOW}⚠ {text}{Colors.END}")

def print_info(text):
    """Выводит информационное сообщение"""
    print(f"{Colors.BLUE}ℹ {text}{Colors.END}")

def detect_distribution():
    """
    Определяет дистрибутив Linux и его версию 
    
    Returns:
        tuple: (distro_name, version_id) или (None, None) при ошибке
    """
    try:
        # Пытаемся прочитать /etc/os-release
        with open('/etc/os-release', 'r') as f:
            lines = f.readlines()
        
        distro_info = {}
        for line in lines:
            if '=' in line:
                key, value = line.strip().split('=', 1)
                distro_info[key] = value.strip('"')
        
        name = distro_info.get('NAME', '').lower()
        version_id = distro_info.get('VERSION_ID', '')
        
        # Определяем тип дистрибутива
        if 'arch' in name:
            return 'arch', version_id
        elif 'ubuntu' in name or 'debian' in name:
            return 'ubuntu', version_id
        elif 'fedora' in name or 'rhel' in name or 'centos' in name:
            return 'fedora', version_id
        else:
            return 'unknown', version_id
            
    except Exception as e:
        print_error(f"Ошибка определения дистрибутива: {e}")
        return None, None

def check_privileges():
    """Проверяет, запущен ли скрипт с правами root"""
    if os.geteuid() != 0:
        print_error("Этот скрипт требует права root. Запустите с sudo!")
        sys.exit(1)

def check_aur_helper():
    """Проверяет наличие AUR helper"""
    helpers = ['yay', 'paru', 'trizen']
    for helper in helpers:
        if shutil.which(helper):
            return helper
    return None

def install_dependencies(distro):
    """Устанавливает основные зависимости для сборки"""
    print_header("УСТАНОВКА ОСНОВНЫХ ЗАВИСИМОСТЕЙ")
    
    if distro == 'arch':
        packages = [
            "base-devel", "git", "wget", "curl",
            "gmp", "mpfr", "mpc", "libmpc", "texinfo",
            "flex", "bison", "gcc", "make", "cmake",
            "nasm", "qemu", "qemu-system-x86", "gdb"
        ]
        try:
            subprocess.run(["pacman", "-S", "--noconfirm"] + packages, check=True)
            print_success("Зависимости установлены успешно")
            return True
        except subprocess.CalledProcessError as e:
            print_error(f"Ошибка установки зависимостей: {e}")
            return False
    
    elif distro == 'ubuntu':
        packages = [
            "build-essential", "git", "wget", "curl",
            "libgmp-dev", "libmpfr-dev", "libmpc-dev", "texinfo",
            "flex", "bison", "gcc", "g++", "make", "cmake",
            "nasm", "qemu-system-x86", "gdb"
        ]
        try:
            subprocess.run(["apt", "update"], check=True)
            subprocess.run(["apt", "install", "-y"] + packages, check=True)
            print_success("Зависимости установлены успешно")
            return True
        except subprocess.CalledProcessError as e:
            print_error(f"Ошибка установки зависимостей: {e}")
            return False
    
    return False

def install_aur_packages():
    """Устанавливает пакеты из AUR"""
    print_header("УСТАНОВКА ПАКЕТОВ ИЗ AUR")
    
    aur_helper = check_aur_helper()
    if not aur_helper:
        print_error("Не найден AUR helper. Установите yay или paru")
        return False
    
    packages = [
        "x86_64-elf-gcc",
        "x86_64-elf-binutils",
        "x86_64-elf-gdb"
    ]
    
    try:
        print_info(f"Используется AUR helper: {aur_helper}")
        for package in packages:
            print_info(f"Установка {package}...")
            subprocess.run([aur_helper, "-S", "--noconfirm", package], check=True)
            print_success(f"{package} установлен успешно")
        return True
    except subprocess.CalledProcessError as e:
        print_error(f"Ошибка установки AUR пакетов: {e}")
        return False

def build_cross_compiler_from_source():
    """Собирает кросс-компилятор из исходников :cite[5]:cite[9]"""
    print_header("СБОРКА КРОСС-КОМПИЛЯТОРА ИЗ ИСХОДНИКОВ")
    
    # Настройки сборки
    target = "x86_64-elf"
    prefix = "/usr/local/cross"
    sources_dir = Path(tempfile.gettempdir()) / "cross-build-sources"
    build_dir = Path(tempfile.gettempdir()) / "cross-build"
    
    # Версии инструментов (можно изменить при необходимости)
    binutils_version = "2.42"
    gcc_version = "13.2.0"
    
    try:
        # Создаем директории
        sources_dir.mkdir(parents=True, exist_ok=True)
        build_dir.mkdir(parents=True, exist_ok=True)
        Path(prefix).mkdir(parents=True, exist_ok=True)
        
        # Устанавливаем переменные окружения
        env = os.environ.copy()
        env["PATH"] = f"{prefix}/bin:{env['PATH']}"
        
        print_info("Скачивание и установка binutils...")
        
        # Скачиваем и распаковываем binutils
        binutils_url = f"https://ftp.gnu.org/gnu/binutils/binutils-{binutils_version}.tar.xz"
        binutils_file = sources_dir / f"binutils-{binutils_version}.tar.xz"
        binutils_dir = sources_dir / f"binutils-{binutils_version}"
        
        subprocess.run(["wget", "-O", str(binutils_file), binutils_url], check=True)
        subprocess.run(["tar", "xf", str(binutils_file), "-C", str(sources_dir)], check=True)
        
        # Собираем binutils
        binutils_build_dir = build_dir / "binutils"
        binutils_build_dir.mkdir(exist_ok=True)
        
        subprocess.run([
            str(binutils_dir / "configure"),
            f"--target={target}",
            f"--prefix={prefix}",
            "--with-sysroot",
            "--disable-nls",
            "--disable-werror"
        ], cwd=str(binutils_build_dir), env=env, check=True)
        
        subprocess.run(["make", "-j", str(os.cpu_count())], 
                      cwd=str(binutils_build_dir), check=True)
        subprocess.run(["make", "install"], cwd=str(binutils_build_dir), check=True)
        
        print_success("Binutils установлен успешно")
        
        print_info("Скачивание и установка GCC...")
        
        # Скачиваем и распаковываем GCC
        gcc_url = f"https://ftp.gnu.org/gnu/gcc/gcc-{gcc_version}/gcc-{gcc_version}.tar.xz"
        gcc_file = sources_dir / f"gcc-{gcc_version}.tar.xz"
        gcc_dir = sources_dir / f"gcc-{gcc_version}"
        
        subprocess.run(["wget", "-O", str(gcc_file), gcc_url], check=True)
        subprocess.run(["tar", "xf", str(gcc_file), "-C", str(sources_dir)], check=True)
        
        # Собираем GCC
        gcc_build_dir = build_dir / "gcc"
        gcc_build_dir.mkdir(exist_ok=True)
        
        subprocess.run([
            str(gcc_dir / "configure"),
            f"--target={target}",
            f"--prefix={prefix}",
            "--disable-nls",
            "--enable-languages=c,c++",
            "--without-headers"
        ], cwd=str(gcc_build_dir), env=env, check=True)
        
        subprocess.run(["make", "all-gcc", "-j", str(os.cpu_count())], 
                      cwd=str(gcc_build_dir), check=True)
        subprocess.run(["make", "all-target-libgcc", "-j", str(os.cpu_count())], 
                      cwd=str(gcc_build_dir), check=True)
        subprocess.run(["make", "install-gcc"], cwd=str(gcc_build_dir), check=True)
        subprocess.run(["make", "install-target-libgcc"], cwd=str(gcc_build_dir), check=True)
        
        print_success("GCC установлен успешно")
        
        # Добавляем в PATH
        with open("/etc/profile.d/cross-compiler.sh", "w") as f:
            f.write(f'export PATH="{prefix}/bin:$PATH"\n')
        
        print_success(f"Кросс-компилятор установлен в {prefix}/bin")
        return True
        
    except Exception as e:
        print_error(f"Ошибка сборки из исходников: {e}")
        return False

def install_ubuntu_cross_compiler():
    """Устанавливает кросс-компилятор на Ubuntu/Debian"""
    print_header("УСТАНОВКА КРОСС-КОМПИЛЯТОРА НА UBUNTU")
    
    packages = [
        "gcc-x86-64-elf",
        "binutils-x86-64-elf",
        "gdb-x86-64-elf"
    ]
    
    try:
        subprocess.run(["apt", "install", "-y"] + packages, check=True)
        print_success("Кросс-компилятор установлен успешно")
        return True
    except subprocess.CalledProcessError:
        print_warning("Не удалось установить кросс-компилятор из репозиториев")
        return False

def install_additional_tools(distro):
    """Устанавливает дополнительные инструменты для OSDev"""
    print_header("УСТАНОВКА ДОПОЛНИТЕЛЬНЫХ ИНСТРУМЕНТОВ")
    
    tools = {
        "arch": [
            "nasm", "qemu", "qemu-system-x86", "gdb",
            "bochs", "mtools", "dosfstools"
        ],
        "ubuntu": [
            "nasm", "qemu-system-x86", "gdb",
            "bochs", "mtools", "dosfstools"
        ]
    }
    
    if distro in tools:
        try:
            if distro == 'arch':
                subprocess.run(["pacman", "-S", "--noconfirm"] + tools[distro], check=True)
            elif distro == 'ubuntu':
                subprocess.run(["apt", "install", "-y"] + tools[distro], check=True)
            
            print_success("Дополнительные инструменты установлены")
            return True
        except subprocess.CalledProcessError as e:
            print_error(f"Ошибка установки дополнительных инструментов: {e}")
            return False
    
    return False

def test_cross_compiler():
    """Проверяет установку кросс-компилятора"""
    print_header("ПРОВЕРКА УСТАНОВКИ КРОСС-КОМПИЛЯТОРА")
    
    success = True
    tools = ["x86_64-elf-gcc", "x86_64-elf-ld", "x86_64-elf-as", "x86_64-elf-gdb"]
    
    for tool in tools:
        if shutil.which(tool):
            print_success(f"{tool}: установлен")
        else:
            print_error(f"{tool}: не установлен")
            success = False
    
    # Проверяем версию GCC
    try:
        result = subprocess.run(["x86_64-elf-gcc", "--version"], 
                              capture_output=True, text=True)
        if result.returncode == 0:
            version_line = result.stdout.split('\n')[0]
            print_success(f"Версия компилятора: {version_line}")
        else:
            print_error("Не удалось получить версию компилятора")
            success = False
    except Exception as e:
        print_error(f"Ошибка проверки версии компилятора: {e}")
        success = False
    
    return success

def show_osdev_info():
    """Показывает информацию об инструментах OSDev"""
    print_header("ИНФОРМАЦИЯ ОБ ИНСТРУМЕНТАХ РАЗРАБОТКИ ОС")
    
    print(f"""
{Colors.BOLD}Установленные инструменты:{Colors.END}

{Colors.GREEN}• x86_64-elf-gcc:{Colors.END} Кросс-компилятор C/C++ для x86_64 архитектуры
{Colors.GREEN}• x86_64-elf-binutils:{Colors.END} Бинарные утилиты (ассемблер, линковщик)
{Colors.GREEN}• NASM:{Colors.END} Современный ассемблер для x86 архитектуры
{Colors.GREEN}• QEMU:{Colors.END} Эмулятор для тестирования ОС
{Colors.GREEN}• GDB:{Colors.END} Отладчик для низкоуровневой отладки

{Colors.BOLD}Следующие шаги:{Colors.END}

1. Создайте структуру проекта:
   my_os/
   ├── boot/          # Код загрузчика
   ├── kernel/        # Ядро ОС  
   ├── lib/           # Системные библиотеки
   └── Makefile       # Система сборки

2. Пример компиляции ядра:
   {Colors.CYAN}x86_64-elf-gcc -std=gnu99 -ffreestanding -nostdlib -c kernel.c -o kernel.o{Colors.END}

3. Запуск в QEMU:
   {Colors.CYAN}qemu-system-x86_64 -kernel my_os.kernel{Colors.END}

{Colors.BOLD}Полезные ресурсы:{Colors.END}
• OSDev Wiki: http://wiki.osdev.org
• GNU Binutils: https://www.gnu.org/software/binutils/
• GCC Cross-Compiler: http://wiki.osdev.org/GCC_Cross-Compiler
    """)

def main():
    """Основная функция скрипта"""
    parser = argparse.ArgumentParser(
        description="Установка инструментов для разработки операционных систем",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
{Colors.BOLD}Примеры использования:{Colors.END}
  {sys.argv[0]}                 # Автоматическая установка
  {sys.argv[0]} --source-build  # Принудительная сборка из исходников
  {sys.argv[0]} --test-only     # Только проверка установленных инструментов
  {sys.argv[0]} --info          # Информация об инструментах OSDev

{Colors.BOLD}Стратегии установки:{Colors.END}
  • AUR пакеты (Arch Linux) - рекомендуется
  • Сборка из исходников - надежный запасной вариант
  • Репозитории Ubuntu - для Debian-based систем

{Colors.BOLD}Решаемые проблемы:{Colors.END}
  • Установка x86_64-elf-gcc и зависимостей
  • Сборка кросс-компилятора при недоступности бинарных пакетов
  • Настройка полного окружения для разработки ОС
        """
    )
    
    parser.add_argument(
        '--source-build', 
        action='store_true',
        help='Принудительная сборка из исходников (рекомендуется при проблемах с AUR)'
    )
    parser.add_argument(
        '--test-only', 
        action='store_true',
        help='Только проверить установленные инструменты'
    )
    parser.add_argument(
        '--info', 
        action='store_true',
        help='Показать информацию об инструментах OSDev'
    )
    
    args = parser.parse_args()
    
    print_header("OS DEVELOPMENT ENVIRONMENT SETUP")
    print(f"{Colors.BLUE}Расширенный скрипт установки инструментов для разработки ОС{Colors.END}")
    
    if args.info:
        show_osdev_info()
        return
    
    if args.test_only:
        test_cross_compiler()
        return
    
    # Проверка прав администратора
    check_privileges()
    
    # Определение дистрибутива
    distro, version = detect_distribution()
    print_info(f"Обнаружен дистрибутив: {distro} {version}")
    
    if distro not in ['arch', 'ubuntu']:
        print_error("Неподдерживаемый дистрибутив")
        print_info("Поддерживаются: Arch Linux, Ubuntu/Debian")
        sys.exit(1)
    
    success = True
    
    # Установка зависимостей
    if not install_dependencies(distro):
        success = False
    
    # Установка кросс-компилятора
    if distro == 'arch' and not args.source_build:
        # Сначала пробуем AUR
        if not install_aur_packages():
            print_warning("Не удалось установить из AUR, пробуем сборку из исходников...")
            if not build_cross_compiler_from_source():
                success = False
    elif distro == 'ubuntu' and not args.source_build:
        # Пробуем репозитории Ubuntu
        if not install_ubuntu_cross_compiler():
            print_warning("Не удалось установить из репозиториев, пробуем сборку из исходников...")
            if not build_cross_compiler_from_source():
                success = False
    else:
        # Принудительная сборка из исходников
        if not build_cross_compiler_from_source():
            success = False
    
    # Установка дополнительных инструментов
    if success:
        install_additional_tools(distro)
    
    # Финальная проверка
    if success:
        print_success("Установка завершена успешно!")
        test_cross_compiler()
        show_osdev_info()
        
        print(f"\n{Colors.GREEN}{Colors.BOLD}Готово к разработке операционной системы! 🎉{Colors.END}")
        print(f"{Colors.YELLOW}Для применения изменений PATH выполните: source /etc/profile{Colors.END}")
    else:
        print_error("Установка завершена с ошибками")
        print_info("Рекомендации:")
        print("1. Проверьте подключение к интернету")
        print("2. Убедитесь, что система обновлена")
        print("3. Попробуйте запустить с --source-build для сборки из исходников")
        sys.exit(1)

if __name__ == "__main__":
    main()