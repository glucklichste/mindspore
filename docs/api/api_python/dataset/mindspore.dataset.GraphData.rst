mindspore.dataset.GraphData
===========================

.. py:class:: mindspore.dataset.GraphData(dataset_file, num_parallel_workers=None, working_mode='local', hostname='127.0.0.1', port=50051, num_client=1, auto_shutdown=True)

    从共享文件和数据库中读取用于GNN训练的图数据集。

    **参数：**

    - **dataset_file** (str) - 数据集文件路径。
    - **num_parallel_workers** (int, 可选) - 读取数据的工作线程数（默认为None）。
    - **working_mode** (str, 可选) - 设置工作模式，目前支持'local'/'client'/'server'（默认为'local'）。

      - **local**：用于非分布式训练场景。
      - **client**：用于分布式训练场景。客户端不加载数据，而是从服务器获取数据。
      - **server**：用于分布式训练场景。服务器加载数据并可供客户端使用。

    - **hostname** (str, 可选) - 图数据集服务器的主机名。该参数仅在工作模式设置为 'client' 或 'server' 时有效（默认为'127.0.0.1'）。
    - **port** (int, 可选) - 图数据服务器的端口，取值范围为1024-65535。此参数仅当工作模式设置为 'client' 或 'server' （默认为50051）时有效。
    - **num_client** (int, 可选) - 期望连接到服务器的最大客户端数。服务器将根据该参数分配资源。该参数仅在工作模式设置为 'server' 时有效（默认为1）。
    - **auto_shutdown** (bool, 可选) - 当工作模式设置为 'server' 时有效。当连接的客户端数量达到 `num_client` ，且没有客户端正在连接时，服务器将自动退出（默认为True）。

    **样例：**

    >>> graph_dataset_dir = "/path/to/graph_dataset_file"
    >>> graph_dataset = ds.GraphData(dataset_file=graph_dataset_dir, num_parallel_workers=2)
    >>> nodes = graph_dataset.get_all_nodes(node_type=1)
    >>> features = graph_dataset.get_node_feature(node_list=nodes, feature_types=[1])


    .. py:method:: get_all_edges(edge_type)

        获取图的所有边。

        **参数：**

        - **edge_type** (int) - 指定边的类型。

        **返回：**

        numpy.ndarray，包含边的数组。

        **样例：**

        >>> edges = graph_dataset.get_all_edges(edge_type=0)

        **异常：**

        **TypeError**：参数 `edge_type` 的类型不为整型。

    .. py:method:: get_all_neighbors(node_list, neighbor_type, output_format=<OutputFormat.NORMAL: 0。

        获取 `node_list` 所有节点的邻居，以 `neighbor_type` 类型返回。格式的定义参见以下示例：1表示两个节点之间连接，0表示不连接。

        .. list-table:: 邻接矩阵
            :widths: 20 20 20 20 20
            :header-rows: 1

            * -
              - 0
              - 1
              - 2
              - 3
            * - 0
              - 0
              - 1
              - 0
              - 0
            * - 1
              - 0
              - 0
              - 1
              - 0
            * - 2
              - 1
              - 0
              - 0
              - 1
            * - 3
              - 1
              - 0
              - 0
              - 0

        .. list-table:: 普通格式
            :widths: 20 20 20 20 20
            :header-rows: 1

            * - src
              - 0
              - 1
              - 2
              - 3
            * - dst_0
              - 1
              - 2
              - 0
              - 1
            * - dst_1
              - -1
              - -1
              - 3
              - -1

        .. list-table:: COO格式
            :widths: 20 20 20 20 20 20
            :header-rows: 1

            * - src
              - 0
              - 1
              - 2
              - 2
              - 3
            * - dst
              - 1
              - 2
              - 0
              - 3
              - 1

        .. list-table:: CSR格式
            :widths: 40 20 20 20 20 20
            :header-rows: 1

            * - offsetTable
              - 0
              - 1
              - 2
              - 4
              -
            * - dstTable
              - 1
              - 2
              - 0
              - 3
              - 1

        **参数：**

        - **node_list** (Union[list, numpy.ndarray]) - 给定的节点列表。
        - **neighbor_type** (int) - 指定邻居节点的类型。
        - **output_format** (OutputFormat, 可选) - 输出存储格式（默认为mindspore.dataset.engine.OutputFormat.NORMAL）取值范围：[OutputFormat.NORMAL, OutputFormat.COO, OutputFormat.CSR]。

        **返回：**

        对于普通格式或COO格式，将返回numpy.ndarray类型的数组表示邻居节点。如果指定了CSR格式，将返回两个numpy.ndarray数组，第一个表示偏移表，第二个表示邻居节点。

        **样例：**

        >>> from mindspore.dataset.engine import OutputFormat
        >>> nodes = graph_dataset.get_all_nodes(node_type=1)
        >>> neighbors = graph_dataset.get_all_neighbors(node_list=nodes, neighbor_type=2)
        >>> neighbors_coo = graph_dataset.get_all_neighbors(node_list=nodes, neighbor_type=2,
        ...                                                 output_format=OutputFormat.COO)
        >>> offset_table, neighbors_csr = graph_dataset.get_all_neighbors(node_list=nodes, neighbor_type=2,
        ...                                                               output_format=OutputFormat.CSR)

        **异常：**

        - **TypeError** - 参数 `node_list` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `neighbor_type` 的类型不为整型。

    .. py:method:: get_all_nodes(node_type)

        获取图中的所有节点。

        **参数：**

        - **node_type** (int) - 指定节点的类型。

        **返回：**

        numpy.ndarray，包含节点的数组。

        **样例：**

        >>> nodes = graph_dataset.get_all_nodes(node_type=1)

        **异常：**

        **TypeError**：参数 `node_type` 的类型不为整型。

    .. py:method:: get_edges_from_nodes(node_list)

        从节点获取边。

        **参数：**

        - **node_list** (Union[list[tuple], numpy.ndarray]) - 含一个或多个图节点ID对的列表。

        **返回：**

        numpy.ndarray，含一个或多个边ID的数组。

        **示例：**

        >>> edges = graph_dataset.get_edges_from_nodes(node_list=[(101, 201), (103, 207)])

        **异常：**

        **TypeError**：参数 `edge_list` 的类型不为列表或numpy.ndarray。

    .. py:method:: get_edge_feature(edge_list, feature_types)

        获取 `edge_list` 列表中边的特征，以 `feature_types` 类型返回。

        **参数：**

        - **edge_list** (Union[list, numpy.ndarray]) - 包含边的列表。
        - **feature_types** (Union[list, numpy.ndarray]) - 包含给定特征类型的列表。

        **返回：**

        numpy.ndarray，包含特征的数组。

        **样例：**

        >>> edges = graph_dataset.get_all_edges(edge_type=0)
        >>> features = graph_dataset.get_edge_feature(edge_list=edges, feature_types=[1])

        **异常：**

        - **TypeError** - 参数 `edge_list` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `feature_types` 的类型不为列表或numpy.ndarray。


    .. py:method:: get_neg_sampled_neighbors(node_list, neg_neighbor_num, neg_neighbor_type)

        获取 `node_list` 列表中节所有点的负样本邻居，以 `neg_neighbor_type` 类型返回。

        **参数：**

        - **node_list** (Union[list, numpy.ndarray]) - 包含节点的列表。
        - **neg_neighbor_num** (int) - 采样的邻居数量。
        - **neg_neighbor_type** (int) - 指定负样本邻居的类型。

        **返回：**

        numpy.ndarray，包含邻居的数组。

        **样例：**

        >>> nodes = graph_dataset.get_all_nodes(node_type=1)
        >>> neg_neighbors = graph_dataset.get_neg_sampled_neighbors(node_list=nodes, neg_neighbor_num=5,
        ...                                                         neg_neighbor_type=2)

        **异常：**

        - **TypeError** - 参数 `node_list` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `neg_neighbor_num` 的类型不为整型。
        - **TypeError** - 参数 `neg_neighbor_type` 的类型不为整型。

    .. py:method:: get_nodes_from_edges(edge_list)

        从图中的边获取节点。

        **参数：**

        - **edge_list** (Union[list, numpy.ndarray]) - 包含边的列表。

        **返回：**

        numpy.ndarray，包含节点的数组。

        **异常：**

        **TypeError：** 参数 `edge_list` 不为列表或ndarray。

    .. py:method:: get_node_feature(node_list, feature_types)

        获取 `node_list` 中节点的特征，以 `feature_types` 类型返回。

        **参数：**

        - **node_list** (Union[list, numpy.ndarray]) - 包含节点的列表。
        - **feature_types** (Union[list, numpy.ndarray]) - 指定特征的类型。

        **返回：**

        numpy.ndarray，包含特征的数组。

        **示例：**

        >>> nodes = graph_dataset.get_all_nodes(node_type=1)
        >>> features = graph_dataset.get_node_feature(node_list=nodes, feature_types=[2, 3])

        **异常：**

        - **TypeError** - 参数 `node_list` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `feature_types` 的类型不为列表或numpy.ndarray。

    .. py:method:: get_sampled_neighbors(node_list, neighbor_nums, neighbor_types, strategy=<SamplingStrategy.RANDOM: 0>)

        获取已采样邻居信息。此API支持多跳邻居采样。即将上一次采样结果作为下一跳采样的输入，最多允许6跳。采样结果平铺成列表，格式为[input node, 1-hop sampling result, 2-hop samling result ...]

        **参数：**

        - **node_list** (Union[list, numpy.ndarray]) - 包含节点的列表。
        - **neighbor_nums** (Union[list, numpy.ndarray]) - 每跳采样的邻居数。
        - **neighbor_types** (Union[list, numpy.ndarray]) - 每跳采样的邻居类型。
        - **strategy** (SamplingStrategy, 可选) - 采样策略（默认为mindspore.dataset.engine.SamplingStrategy.RANDOM）。取值范围：[SamplingStrategy.RANDOM, SamplingStrategy.EDGE_WEIGHT]。

        - **SamplingStrategy.RANDOM**：随机抽样，带放回采样。
        - **SamplingStrategy.EDGE_WEIGHT**：以边缘权重为概率进行采样。

        **返回：**

        numpy.ndarray，包含邻居的数组。

        *样例：**

        >>> nodes = graph_dataset.get_all_nodes(node_type=1)
        >>> neighbors = graph_dataset.get_sampled_neighbors(node_list=nodes, neighbor_nums=[2, 2],
        ...                                                 neighbor_types=[2, 1])

        **异常：**

        - **TypeError** - 参数 `node_list` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `neighbor_nums` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `neighbor_types`  的类型不为列表或numpy.ndarray。


    .. py:method:: graph_info()

        获取图的元信息，包括节点数、节点类型、节点特征信息、边数、边类型、边特征信息。

        **返回：**

        dict，图的元信息。键为 `node_num` 、 `node_type` 、 `node_feature_type` 、 `edge_num` 、 `edge_type` 、和 `edge_feature_type` 。


    .. py:method:: random_walk(target_nodes, meta_path, step_home_param=1.0, step_away_param=1.0, default_node=-1)

        在节点中的随机游走。

        **参数：**

        - **target_nodes** (list[int]) - 随机游走中的起始节点列表。
        - **meta_path** (list[int]) - 每个步长的节点类型。
        - **step_home_param** (float, 可选) - 返回node2vec算法中的超参（默认为1.0）。
        - **step_away_param** (float, 可选) - node2vec算法中的in和out超参（默认为1.0）。
        - **default_node** (int, 可选) - 如果找不到更多邻居，则为默认节点（默认值为-1，表示不给定节点）。

        **返回：**

        numpy.ndarray，包含节点的数组。

        **示例：**

        >>> nodes = graph_dataset.get_all_nodes(node_type=1)
        >>> walks = graph_dataset.random_walk(target_nodes=nodes, meta_path=[2, 1, 2])

        **异常：**

        - **TypeError** - 参数 `target_nodes` 的类型不为列表或numpy.ndarray。
        - **TypeError** - 参数 `meta_path` 的类型不为列表或numpy.ndarray。
