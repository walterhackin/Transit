# Грузовики

В данном проекте полная реализация задачи о перевозке железной руды на Урале.
Репозиторий будет постоянно обновляться.

## Общая информация

### Разделение задания на подзадачи

1. Реализация SystemFile;
2. Реализация программы, исполняющей роль одного самосвала;
3. Реализация программы, исполняющей роль регулировщика с одной стороны моста;
4. Реализация программы, отвечающей за запуск потоков регулировщиков и самосвалов.

### Используемые инструменты

1. Очереди сообщений для сообщения регулировщикам о прибытии самосвала;
2. Семафоры для передачи самосвалам разрешения на проезд.

### Возможные события

1. У моста нет самосвалов;
2. С одной стороны моста нет самосвалов;
3. Самосвалы есть по обе стороны моста;
4. Прибыл самосвал недопустимого веса.

### Упрощенная задача

1. Все самосвалы одного веса;
2. Регулировщик допускает на мосту наличие только одного самосвала.

### Распределение между участниками группы

1. Шоислом;
2. Вальтер;
3. Глеб;
4. Аян.

## Межпроцессное взаимодействие

### 1. Используемые обозначения
* Самосвал - #2;
* Регулировщик - #3;

### 2. Введение в методы взаимодействия в данной задаче
1. #### Очереди сообщений
   1. Самосвал - Регулировщик:
      * Регулировщик выступает выступает в качестве сервера, Самосвал - в качестве клиента;
      * Самосвал посылает сообщение Регулировщику, расположенному с ним по одну сторону моста, после подъезда к мосту, а Регулировщик принимает его.
2. #### Семафоры
   1. Самосвал - Регулировщик:
      * Регулировщик разрешает Самосвалу проезд по мосту, устанавливая заданное значение семафора;
      * После проезда по мосту Самосвал меняет значение семафора на исходное (исходное значение == 1).
   2. Регулировщик - Регулировщик:
      * Применяется для определения того, какой Регулировщик руководит движением на мосту в данный момент.

### 3. Необходимые данные для корректного взаимодействия, содержащиеся в процессах
1. #### Самосвал
   * Собственный ключ семафора самосвала;
   * Ключ очереди сообщений первого и второго регулировщиков;
   * Масса самосвала;
   * Расположение самосвала относительно моста.
2. #### Регулировщик
   * Собственный ключ очереди сообщений;
   * (Собственный) Ключ семафора регулировщиков;
   * Расположение регулировщика относительно моста ({0, 2}).

### 4. Порядок взаимодействия с "соседними" процессами
1. #### Самосвал - Регулировщик
   1. После подъезда к мосту Самосвал посылает соответствующему Регулировщику сообщение по форме ```"Ключ семафора Самосвала" "Масса Самосвала"```, а затем ждёт разрешение на проезд от Регулировщика;
   2. Если Регулировщик разрешает Самосвалу движение по мосту, то меняет значение его семафора на 0 (уменьшает на 1).
   3. После выезда с моста Самосвал меняет значение семафора на 1 (добавляет 1) и ждёт, пока Регулировщик не установит значение 0;
   4. Регулировщик меняет значение семафора Самосвала на 0 (уменьшает на 1);
   5. Самосвал меняет значение семафора на 1 (добавляет 1).
2. #### Регулировщик - Регулировщик
   1. Для передачи управления движением Регулировщик меняет значение семафора на номер другого регулировщика, а затем ждёт, пока семафор не примет значение, соответствующее его номеру.

## Требования к реализации программ (задач)
### 1. Инициализатор (#4)
1. Реализация в файле ```initialization.cpp```;
2. Использование ```fork()``` для создания процессов;
3. Перед тем, как запустить дочерние процессы, Инициализатор создаёт очереди сообщений для первого и второго регулировщика;
4. Для запуска процессов инициализатор использует функции из ```initialization.hpp```;
5. Программа логирует все свои действия в заданную директорию в файл с названием ```LogInitializator.txt```.

### 2. Самосвал (#2)
1. Объявление класса ```Truck``` в файле ```truck.hpp```;
2. Реализация в файле ```truck.cpp```;
2. Для создания нового самосвала вызывается функция ```TruckFoo```, объявленная в ```initialization.hpp``` (функцию вызывает Инициализатор);
3. Программа логирует все свои действия в заданную директорию в файл с названием ```LogTruck"номер самосвала(ключ семафора)".txt```;
4. Для передачи сообщений использует структуру ```MessageBuffer```, объявленную в ```initialization.hpp```.

### 3. Регулировщик (#3)
1. Объявление класса ```TrafficController``` в файле ```traffic_controller.hpp```;
2. Реализация в файле ```traffic_controller.cpp```;
2. Для создания нового самосвала вызывается функция ```TrafficControllerFoo```, объявленная в ```initialization.hpp``` (функцию вызывает Инициализатор);
3. Программа логирует все свои действия в заданную директорию в файл с названием ```LogTrafficController"расположение регулировщика".txt```;
4. Для передачи сообщений использует структуру ```MessageBuffer```, объявленную в ```initialization.hpp```.
