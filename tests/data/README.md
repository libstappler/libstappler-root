# Тест производительности форматов данных

Тест служит для сравнения производительности кодирования форматов JSON и CBOR в режимах пулового и стандартного распределения памяти для данных разного характера, а также сжатия LZ4 и Brotli.

## Сборка

```
make
make install
```

## Результаты в репозитории

Результат (result.xlsx) получен на на ОС Ubuntu 22.04.2 LTS. ЦП AMD Ryzen 9 7950x. Компилятор GCC 11.3.0, режим оптимизации -O3.

Для сжатия Brotli уровень сжатия снижен на одну условную единицу (с 11 до 10), поскольку уровень сжатия по умолчанию давал неадекватные результаты по скорости работы (более 1000% замедления). Для lz4 используется максимальный уровень сжатия.

## Наборы данных

Тестирование проводилось на двух наборах данных.

Первый набор данных хранится в директории tests/data/json1 репозитория libstappler-root и представляет из себя содержание научного журнала, закодированное для представления на веб-странице. Этот набор данных теоретически достаточно близок к используемым в реальной практике данным в сетевых приложениях. Размер набора данных: 6.6 Мб, 13 объектов.

Второй набор данных - сериализованный исходный код заголовочных файлов SDK, хранящийся в репозитории libstappler-doc (https://github.com/libstappler/libstappler-doc) в директории json. Характер этих данных значительно отличается от первого набора, потому его решено использовать в качестве контрольного. Размер набора данных: 44 Мб, 126 объектов.

## Характер тестирования

На каждом проходе теста измеряется скорость чтения данных из предложенного формата во внутренний тип Value, затем, скорость записи в этот формат, что аналогично стандартному способу использования данных сервером. Чтение и запись производятся из/в оперативную память. Каждый проход выполняется несколько раз (16 в представленных данных) для получения средних значений и вероятных отклонений.

Проходы теста классифицированы по модели памяти:

* Pool - модель пулов памяти
* Std - стандартная модель памяти

Также, по формату:

* Pretty - JSON в человекочитаемом виде
* Json - JSON в машиночитаемом (компактном) виде
* Json/lz4 - JSON в машиночитаемом виде, сжатый алгоритмом lz4
* Json/lz4HC - JSON в машиночитаемом виде, сжатый алгоритмом lz4 с опцией высокой компрессии
* Json/Brotli - JSON в машиночитаемом виде, сжатый алгоритмом Brotli
* Cbor - CBOR
* Cbor/lz4 - CBOR, сжатый алгоритмом lz4
* Cbor/lz4HC - CBOR, сжатый алгоритмом lz4 с опцией высокой компрессии
* Cbor/Brotli - CBOR, сжатый алгоритмом Brotli
