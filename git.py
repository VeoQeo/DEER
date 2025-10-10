#!/usr/bin/env python3
"""
Version Control Github (VCG)

Упрощенный инструмент для управления Git репозиториями
с красивым интерфейсом и расширенными функциями
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from typing import List, Optional, Tuple
import json
from datetime import datetime

class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    PURPLE = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

class VCGSystem:
    def __init__(self):
        self.config_file = '.vcg_config.json'
        self.load_config()
    
    def load_config(self):
        self.config = {
            'auto_push': True,
            'default_remote': 'origin',
            'default_branch': 'main',
            'editor': os.getenv('EDITOR', 'nano')
        }
        if os.path.exists(self.config_file):
            try:
                with open(self.config_file, 'r') as f:
                    self.config.update(json.load(f))
            except:
                pass
    
    def save_config(self):
        with open(self.config_file, 'w') as f:
            json.dump(self.config, f, indent=2)
    
    def print_banner(self):
        banner = f"""
{Colors.PURPLE}{Colors.BOLD}
╦  ╦╔═╗╔╦╗╔═╗╦ ╦  ╔═╗╔╦╗╔═╗╦ ╦╔═╗
║  ║╠═╣║║║╠═╣╚╦╝  ║ ║ ║ ║ ║║║║╔═╝
╩═╝╩╩ ╩╩ ╩╩ ╩ ╩   ╚═╝ ╩ ╚═╝╚╩╝╚═╝
                        
        Version Control Github
      Упрощенное управление Git
{Colors.END}"""
        print(banner)
    
    def run_git_command(self, command: List[str], capture_output: bool = False) -> Tuple[bool, str]:
        try:
            if capture_output:
                result = subprocess.run(['git'] + command, 
                                      capture_output=True, 
                                      text=True, 
                                      check=True)
                return True, result.stdout.strip()
            else:
                subprocess.run(['git'] + command, check=True)
                return True, ""
        except subprocess.CalledProcessError as e:
            return False, e.stderr if capture_output else str(e)
    
    def check_git_repo(self) -> bool:
        return os.path.exists('.git')
    
    def ensure_repo(self):
        if not self.check_git_repo():
            self.log("❌ Не найден Git репозиторий. Инициализируем...", "error")
            success, msg = self.run_git_command(['init'])
            if success:
                self.log("✅ Git репозиторий инициализирован", "success")
            else:
                self.log(f"❌ Ошибка инициализации: {msg}", "error")
                sys.exit(1)
    
    def log(self, message: str, level: str = "info"):
        colors = {
            "info": Colors.BLUE,
            "success": Colors.GREEN,
            "warning": Colors.YELLOW,
            "error": Colors.RED,
            "debug": Colors.CYAN
        }
        icons = {
            "info": "ℹ️",
            "success": "✅",
            "warning": "⚠️",
            "error": "❌",
            "debug": "🐛"
        }
        print(f"{colors.get(level, '')}{icons.get(level, '')} {message}{Colors.END}")
    
    def init_repo(self, remote_url: str = None):
        self.ensure_repo()
        
        if remote_url:
            success, msg = self.run_git_command(['remote', 'add', 'origin', remote_url])
            if success:
                self.log(f"✅ Добавлен удаленный репозиторий: {remote_url}", "success")
            else:
                self.log(f"❌ Ошибка добавления remote: {msg}", "error")
        
        self.log("📁 Репозиторий готов к работе", "success")
    
    def status(self):
        self.ensure_repo()
        self.log("📊 Статус репозитория:", "info")
        success, msg = self.run_git_command(['status', '-s'], capture_output=True)
        if success:
            if msg:
                print(msg)
            else:
                self.log("✅ Нет изменений для коммита", "success")
        else:
            self.log(f"❌ Ошибка: {msg}", "error")
    
    def add_files(self, files: List[str] = None):
        self.ensure_repo()
        
        if not files:
            files = ['.']
        
        for file_pattern in files:
            success, msg = self.run_git_command(['add', file_pattern])
            if success:
                if file_pattern == '.':
                    self.log("✅ Все файлы добавлены в stage", "success")
                else:
                    self.log(f"✅ Файл(ы) '{file_pattern}' добавлены в stage", "success")
            else:
                self.log(f"❌ Ошибка добавления '{file_pattern}': {msg}", "error")
    
    def commit(self, message: str = None, amend: bool = False):
        self.ensure_repo()
        
        if not message:
            message = f"Авто-коммит {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
        
        command = ['commit', '-m', message]
        if amend:
            command.append('--amend')
        
        success, msg = self.run_git_command(command)
        if success:
            self.log(f"✅ Коммит создан: {message}", "success")
            
            if self.config['auto_push']:
                self.push()
        else:
            self.log(f"❌ Ошибка коммита: {msg}", "error")
    
    def quick_commit(self, message: str):
        self.add_files(['.'])
        self.commit(message)
    
    def create_branch(self, branch_name: str, switch: bool = True):
        self.ensure_repo()
        
        success, msg = self.run_git_command(['branch', branch_name])
        if success:
            self.log(f"✅ Ветка '{branch_name}' создана", "success")
            
            if switch:
                self.switch_branch(branch_name)
        else:
            self.log(f"❌ Ошибка создания ветки: {msg}", "error")
    
    def switch_branch(self, branch_name: str, create: bool = False):
        self.ensure_repo()
        
        command = ['checkout', branch_name]
        if create:
            command.insert(1, '-b')
        
        success, msg = self.run_git_command(command)
        if success:
            action = "создана и активирована" if create else "активирована"
            self.log(f"✅ Ветка '{branch_name}' {action}", "success")
        else:
            self.log(f"❌ Ошибка переключения ветки: {msg}", "error")
    
    def list_branches(self):
        self.ensure_repo()
        
        self.log("🌿 Доступные ветки:", "info")
        success, msg = self.run_git_command(['branch', '-a'], capture_output=True)
        if success:
            current_branch = None
            for line in msg.split('\n'):
                if line.startswith('*'):
                    current_branch = line[1:].strip()
                    print(f"{Colors.GREEN}* {current_branch}{Colors.END}")
                else:
                    print(f"  {line.strip()}")
        else:
            self.log(f"❌ Ошибка получения списка веток: {msg}", "error")
    
    def merge_branch(self, branch_name: str, no_ff: bool = False):
        self.ensure_repo()
        
        command = ['merge', branch_name]
        if no_ff:
            command.append('--no-ff')
        
        success, msg = self.run_git_command(command)
        if success:
            self.log(f"✅ Ветка '{branch_name}' успешно смержена", "success")
        else:
            self.log(f"❌ Ошибка слияния: {msg}", "error")
    
    def pull(self, rebase: bool = False):
        self.ensure_repo()
        
        command = ['pull']
        if rebase:
            command.append('--rebase')
        
        success, msg = self.run_git_command(command)
        if success:
            self.log("✅ Изменения получены успешно", "success")
        else:
            self.log(f"❌ Ошибка pull: {msg}", "error")
    
    def push(self, branch: str = None, force: bool = False):
        self.ensure_repo()
        
        if not branch:
            success, current_branch = self.run_git_command(
                ['branch', '--show-current'], 
                capture_output=True
            )
            if success:
                branch = current_branch
            else:
                branch = self.config['default_branch']
        
        command = ['push']
        if force:
            command.append('--force')
        command.extend([self.config['default_remote'], branch])
        
        success, msg = self.run_git_command(command)
        if success:
            self.log(f"✅ Изменения отправлены в {self.config['default_remote']}/{branch}", "success")
        else:
            self.log(f"❌ Ошибка push: {msg}", "error")
    
    def show_log(self, graph: bool = False, oneline: bool = False):
        self.ensure_repo()
        
        command = ['log']
        if graph and oneline:
            command.extend(['--graph', '--oneline', '--all'])
        elif graph:
            command.extend(['--graph', '--all'])
        elif oneline:
            command.append('--oneline')
        
        success, msg = self.run_git_command(command, capture_output=True)
        if success:
            if msg:
                print(msg)
            else:
                self.log("📭 История коммитов пуста", "info")
        else:
            self.log(f"❌ Ошибка получения логов: {msg}", "error")
    
    def stash_changes(self, message: str = None):
        self.ensure_repo()
        
        command = ['stash', 'push']
        if message:
            command.extend(['-m', message])
        
        success, msg = self.run_git_command(command)
        if success:
            self.log("💾 Изменения сохранены в stash", "success")
        else:
            self.log(f"❌ Ошибка stash: {msg}", "error")
    
    def pop_stash(self):
        self.ensure_repo()
        
        success, msg = self.run_git_command(['stash', 'pop'])
        if success:
            self.log("📤 Изменения восстановлены из stash", "success")
        else:
            self.log(f"❌ Ошибка pop stash: {msg}", "error")
    
    def set_upstream(self, branch: str = None):
        self.ensure_repo()
        
        if not branch:
            success, current_branch = self.run_git_command(
                ['branch', '--show-current'], 
                capture_output=True
            )
            if success:
                branch = current_branch
        
        success, msg = self.run_git_command(
            ['push', '--set-upstream', self.config['default_remote'], branch]
        )
        if success:
            self.log(f"✅ Upstream установлен для ветки {branch}", "success")
        else:
            self.log(f"❌ Ошибка установки upstream: {msg}", "error")

def main():
    vcg = VCGSystem()
    vcg.print_banner()
    
    parser = argparse.ArgumentParser(
        description='Version Control Github - Упрощенное управление Git',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
{Colors.BOLD}Примеры использования:{Colors.END}
  {sys.argv[0]} init                          # Инициализировать репозиторий
  {sys.argv[0]} status                        # Показать статус
  {sys.argv[0]} add                           # Добавить все файлы
  {sys.argv[0]} commit -m "Сообщение"         # Создать коммит
  {sys.argv[0]} quick "Сообщение"             # Быстрый коммит всех изменений
  {sys.argv[0]} branch create new-feature     # Создать новую ветку
  {sys.argv[0]} branch switch main            # Переключиться на ветку
  {sys.argv[0]} branch list                   # Показать все ветки
  {sys.argv[0]} log --graph                   # Показать историю с графом
  {sys.argv[0]} push                          # Отправить изменения
  {sys.argv[0]} pull --rebase                 # Получить изменения с rebase

{Colors.BOLD}Рекомендуемый рабочий процесс:{Colors.END}
  1. {Colors.CYAN}git.py status{Colors.END}                    - Проверить изменения
  2. {Colors.CYAN}git.py add{Colors.END}                       - Добавить файлы
  3. {Colors.CYAN}git.py commit -m "описание"{Colors.END}      - Создать коммит
  4. {Colors.CYAN}git.py push{Colors.END}                      - Отправить на сервер
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Команды')
    
    # Init command
    init_parser = subparsers.add_parser('init', help='Инициализировать репозиторий')
    init_parser.add_argument('--remote', help='URL удаленного репозитория')
    
    # Status command
    subparsers.add_parser('status', help='Показать статус репозитория')
    
    # Add command
    add_parser = subparsers.add_parser('add', help='Добавить файлы в stage')
    add_parser.add_argument('files', nargs='*', default=['.'], help='Файлы для добавления')
    
    # Commit command
    commit_parser = subparsers.add_parser('commit', help='Создать коммит')
    commit_parser.add_argument('-m', '--message', required=True, help='Сообщение коммита')
    commit_parser.add_argument('--amend', action='store_true', help='Изменить последний коммит')
    
    # Quick commit command
    quick_parser = subparsers.add_parser('quick', help='Быстрый коммит всех изменений')
    quick_parser.add_argument('message', help='Сообщение коммита')
    
    # Branch commands
    branch_parser = subparsers.add_parser('branch', help='Управление ветками')
    branch_subparsers = branch_parser.add_subparsers(dest='branch_command', help='Команды веток')
    
    branch_create_parser = branch_subparsers.add_parser('create', help='Создать ветку')
    branch_create_parser.add_argument('name', help='Название ветки')
    branch_create_parser.add_argument('--no-switch', action='store_true', help='Не переключаться на ветку')
    
    branch_switch_parser = branch_subparsers.add_parser('switch', help='Переключиться на ветку')
    branch_switch_parser.add_argument('name', help='Название ветки')
    branch_switch_parser.add_argument('--create', action='store_true', help='Создать если не существует')
    
    branch_subparsers.add_parser('list', help='Показать все ветки')
    
    # Merge command
    merge_parser = subparsers.add_parser('merge', help='Слить ветку')
    merge_parser.add_argument('branch', help='Ветка для слияния')
    merge_parser.add_argument('--no-ff', action='store_true', help='Отключить fast-forward')
    
    # Log command
    log_parser = subparsers.add_parser('log', help='Показать историю коммитов')
    log_parser.add_argument('--graph', action='store_true', help='Показать граф')
    log_parser.add_argument('--oneline', action='store_true', help='Компактный вывод')
    
    # Push command
    push_parser = subparsers.add_parser('push', help='Отправить изменения')
    push_parser.add_argument('--branch', help='Ветка для отправки')
    push_parser.add_argument('--force', action='store_true', help='Принудительная отправка')
    
    # Pull command
    pull_parser = subparsers.add_parser('pull', help='Получить изменения')
    pull_parser.add_argument('--rebase', action='store_true', help='Использовать rebase')
    
    # Stash commands
    stash_parser = subparsers.add_parser('stash', help='Управление stash')
    stash_parser.add_argument('--message', help='Сообщение для stash')
    
    pop_parser = subparsers.add_parser('pop', help='Восстановить из stash')
    
    # Upstream command
    upstream_parser = subparsers.add_parser('upstream', help='Установить upstream')
    upstream_parser.add_argument('--branch', help='Ветка для upstream')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    try:
        if args.command == 'init':
            vcg.init_repo(args.remote)
        
        elif args.command == 'status':
            vcg.status()
        
        elif args.command == 'add':
            vcg.add_files(args.files)
        
        elif args.command == 'commit':
            vcg.commit(args.message, args.amend)
        
        elif args.command == 'quick':
            vcg.quick_commit(args.message)
        
        elif args.command == 'branch':
            if args.branch_command == 'create':
                vcg.create_branch(args.name, not args.no_switch)
            elif args.branch_command == 'switch':
                vcg.switch_branch(args.name, args.create)
            elif args.branch_command == 'list':
                vcg.list_branches()
            else:
                branch_parser.print_help()
        
        elif args.command == 'merge':
            vcg.merge_branch(args.branch, args.no_ff)
        
        elif args.command == 'log':
            vcg.show_log(args.graph, args.oneline)
        
        elif args.command == 'push':
            vcg.push(args.branch, args.force)
        
        elif args.command == 'pull':
            vcg.pull(args.rebase)
        
        elif args.command == 'stash':
            vcg.stash_changes(args.message)
        
        elif args.command == 'pop':
            vcg.pop_stash()
        
        elif args.command == 'upstream':
            vcg.set_upstream(args.branch)
        
        else:
            parser.print_help()
    
    except KeyboardInterrupt:
        vcg.log("\n⏹️ Операция прервана пользователем", "warning")
    except Exception as e:
        vcg.log(f"💥 Неожиданная ошибка: {e}", "error")

if __name__ == '__main__':
    main()