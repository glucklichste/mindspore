mindspore.dataset.NumpySlicesDataset
=====================================

.. py:class:: mindspore.dataset.NumpySlicesDataset(data, column_names=None, num_samples=None, num_parallel_workers=1, shuffle=None, sampler=None, num_shards=None, shard_id=None)

    由Python数据构建源数据集。生成的数据集的列名和列类型取决于用户传入的Python数据。

    **参数：**

    - **data** (Union[list, tuple, dict]) - 输入的Python数据。支持的数据类型包括：list、tuple、dict和其他NumPy格式。输入数据将沿着第一个维度切片，并生成额外的行。如果输入是单个list，则将生成一个数据列，若是嵌套多个list，则生成多个数据列。不建议通过这种方式加载大量的数据，因为可能会在数据加载到内存时等待较长时间。
    - **column_names** (list[str], 可选): 指定数据集生成的列名（默认值为None）。如果未指定列名称，且当输入数据的类型是dict时，输出列名称将被命名为dict的键名，否则它们将被命名为column_0，column_1...。
    - **num_samples** (int, 可选): 指定从数据集中读取的样本数（默认值为None，所有样本）。
    - **num_parallel_workers** (int, 可选): 指定读取数据的工作线程数（默认值为1）。
    - **shuffle** (bool, 可选): 是否混洗数据集。只有输入的 `data` 参数带有可随机访问属性（__getitem__）时，才可以指定该参数。（默认值为None，下表中会展示不同配置的预期行为）。
    - **sampler** (Union[Sampler, Iterable], 可选): 指定从数据集中选取样本的采样器。只有输入的 `data` 参数带有可随机访问属性（__getitem__）时，才可以指定该参数（默认值为None，下表中会展示不同配置的预期行为）。
    - **num_shards** (int, 可选): 分布式训练时，将数据集划分成指定的分片数（默认值None）。指定此参数后，`num_samples` 表示每个分片的最大样本数。需要输入 `data` 支持可随机访问才能指定该参数。
    - **shard_id** (int, 可选): 分布式训练时，指定使用的分片ID号（默认值None）。只有当指定了 `num_shards` 时才能指定此参数。

    .. note::  此数据集可以指定 `sampler` 参数，但 `sampler` 和 `shuffle` 是互斥的。下表展示了几种合法的输入参数及预期的行为。

    .. list-table:: 配置 `sampler` 和 `shuffle` 的不同组合得到的预期排序结果
       :widths: 25 25 50
       :header-rows: 1

       * - 参数 `sampler`
         - 参数 `shuffle`
         - 预期数据顺序
       * - None
         - None
         - 随机排列
       * - None
         - True
         - 随机排列
       * - None
         - False
         - 顺序排列
       * - 参数 `sampler`
         - None
         - 由 `sampler` 行为定义的顺序
       * - 参数 `sampler`
         - True
         - 不允许
       * - 参数 `sampler`
         - False
         - 不允许

    **异常：**

    - **RuntimeError** - `column_names` 列表的长度与数据的输出列表长度不匹配。
    - **RuntimeError** - `num_parallel_workers` 超过系统最大线程数。
    - **RuntimeError** - 同时指定了 `sampler` 和 `shuffle` 。
    - **RuntimeError** - 同时指定了 `sampler` 和 `num_shards` 。
    - **RuntimeError** - 指定了 `num_shards` 参数，但是未指定 `shard_id` 参数。
    - **RuntimeError** - 指定了 `shard_id` 参数，但是未指定 `num_shards` 参数。
    - **ValueError** - `shard_id` 参数错误（小于0或者大于等于 `num_shards` ）。

    **样例：**

    >>> # 1) 输入的 `data` 参数类型为list
    >>> data = [1, 2, 3]
    >>> dataset = ds.NumpySlicesDataset(data=data, column_names=["column_1"])
    >>>
    >>> # 2) 输入的 `data` 参数类型为dict，并且使用column_names的默认行为，即采用键名作为生成列名。
    >>> data = {"a": [1, 2], "b": [3, 4]}
    >>> dataset = ds.NumpySlicesDataset(data=data)
    >>>
    >>> # 3) 输入的 `data` 参数类型是由list组成的tuple（或NumPy数组），每个元组分别生成一个输出列，共三个输出列
    >>> data = ([1, 2], [3, 4], [5, 6])
    >>> dataset = ds.NumpySlicesDataset(data=data, column_names=["column_1", "column_2", "column_3"])
    >>>
    >>> # 4) 从CSV文件加载数据
    >>> import pandas as pd
    >>> df = pd.read_csv(filepath_or_buffer=csv_dataset_dir[0])
    >>> dataset = ds.NumpySlicesDataset(data=dict(df), shuffle=False)

    .. include:: mindspore.dataset.Dataset.add_sampler.rst

    .. include:: mindspore.dataset.Dataset.rst

    .. include:: mindspore.dataset.Dataset.use_sampler.rst

    .. include:: mindspore.dataset.Dataset.zip.rst
