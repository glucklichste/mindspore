mindspore.nn.TrainOneStepWithLossScaleCell
==========================================

.. py:class:: mindspore.nn.TrainOneStepWithLossScaleCell(network, optimizer, scale_sense)

    使用梯度放大功能（loss scale）的训练网络。

    实现了包含梯度放大功能的单次训练。它使用网络、优化器和用于更新梯度放大系数的Cell(或一个Tensor)作为参数。可在host侧或device侧更新梯度放大系数。
    如果需要在host侧更新，使用Tensor作为 `scale_sense` ，否则，使用可更新梯度放大系数的Cell实例作为 `scale_sense` 。

    **参数：**

    - **network** (Cell) - 训练网络。仅支持单输出网络。
    - **optimizer** (Cell) - 用于更新网络参数的优化器。
    - **scale_sense** (Union[Tensor, Cell]) - 如果此值为Cell类型，`TrainOneStepWithLossScaleCell` 会调用它来更新梯度放大系数。如果此值为Tensor类型，可调用 `set_sense_scale` 来更新梯度放大系数，shape为 :math:`()` 或 :math:`(1,)` 。

    **输入：**

    **(*inputs)** (Tuple(Tensor))- shape为 :math:`(N, \ldots)` 的Tensor组成的元组。

    **输出：**

    Tuple，包含三个Tensor，分别为损失函数值、溢出状态和当前梯度放大系数。

    - **loss** （Tensor） - shape为 :math:`()` 的Tensor。
    - **overflow** （Tensor）- shape为 :math:`()` 的Tensor，类型为bool。
    - **loss scale** （Tensor）- shape为 :math:`()` 的Tensor。

    **异常：**

    - **TypeError** - `scale_sense` 既不是Cell，也不是Tensor。
    - **ValueError** - `scale_sense` 的shape既不是(1,)也不是()。

    **支持平台：**

    ``Ascend`` ``GPU``

    **样例：**

    >>> import numpy as np
    >>> import mindspore
    >>> from mindspore import Tensor, Parameter, nn, ops
    >>> from mindspore import dtype as mstype
    >>>
    >>> class Net(nn.Cell):
    ...     def __init__(self, in_features, out_features):
    ...         super(Net, self).__init__()
    ...         self.weight = Parameter(Tensor(np.ones([in_features, out_features]).astype(np.float32)),
    ...                                 name='weight')
    ...         self.matmul = ops.MatMul()
    ...
    ...     def construct(self, x):
    ...         output = self.matmul(x, self.weight)
    ...         return output
    ...
    >>> size, in_features, out_features = 16, 16, 10
    >>> #1）scale_sense类型为Cell时：
    >>> net = Net(in_features, out_features)
    >>> loss = nn.MSELoss()
    >>> optimizer = nn.Momentum(net.trainable_params(), learning_rate=0.1, momentum=0.9)
    >>> net_with_loss = nn.WithLossCell(net, loss)
    >>> manager = nn.DynamicLossScaleUpdateCell(loss_scale_value=2**12, scale_factor=2, scale_window=1000)
    >>> train_network = nn.TrainOneStepWithLossScaleCell(net_with_loss, optimizer, scale_sense=manager)
    >>> input = Tensor(np.ones([out_features, in_features]), mindspore.float32)
    >>> labels = Tensor(np.ones([out_features,]), mindspore.float32)
    >>> output = train_network(input, labels)
    >>>
    >>>> #2）当scale_sense类型为Tensor时：
    >>> net = Net(in_features, out_features)
    >>> loss = nn.MSELoss()
    >>> optimizer = nn.Momentum(net.trainable_params(), learning_rate=0.1, momentum=0.9)
    >>> net_with_loss = nn.WithLossCell(net, loss)
    >>> inputs = Tensor(np.ones([size, in_features]).astype(np.float32))
    >>> label = Tensor(np.zeros([size, out_features]).astype(np.float32))
    >>> scaling_sens = Tensor(np.full((1), np.finfo(np.float32).max), dtype=mstype.float32)
    >>> train_network = nn.TrainOneStepWithLossScaleCell(net_with_loss, optimizer, scale_sense=scaling_sens)
    >>> output = train_network(inputs, label)

    .. py:method:: get_overflow_status(status, compute_output)

        获取浮点溢出状态。

        溢出检测的目标过程执行完成后，获取溢出结果。继承该类自定义训练网络时，可复用该接口。

        **输入：**

        - **status** (object) - 用于检测溢出的状态实例。
        - **compute_output** - 对特定计算过程进行溢出检测时，将 `compute_output` 设置为该计算过程的输出，以确保在执行计算之前获取了 `status`。

        **输出：**

        bool，是否发生溢出。


    .. py:method:: process_loss_scale(overflow)

        根据溢出状态计算梯度放大系数。继承该类自定义训练网络时，可复用该接口。

       **输入：**

       - **overflow** (bool) - 是否发生溢出。

       **输出：**

       bool，溢出状态，即输入。


    .. py:method:: set_sense_scale(sens)

        如果使用了Tensor类型的 `scale_sense` ，可调用此函数修改它的值。

        **输入：**

        - **sens** （Tensor）- 新的梯度放大系数，其shape和类型需要与原始 `scale_sense` 相同。

    .. py:method:: start_overflow_check(pre_cond, compute_input)

        启动浮点溢出检测。创建并清除溢出检测状态。

        指定参数 `pre_cond` 和 `compute_input` ，以确保在正确的时间清除溢出状态。以当前接口为例，我们需要在损失函数计算后进行清除状态，在梯度计算过程中检测溢出。在这种情况下，`pre_cond` 应为损失函数的输出，而 `compute_input` 应为梯度计算函数的输入。继承该类自定义训练网络时，可复用该接口。

        **输入：**

        - **pre_cond** (Tensor) -启动溢出检测的先决条件。它决定溢出状态清除和先前处理的执行顺序。它确保函数 `start_overflow` 在执行完先决条件后清除状态。
        - **compute_input** (object) - 后续运算的输入。需要对特定的计算过程进行溢出检测。将 `compute_input` 设置这一计算过程的输入，以确保在执行该计算之前清除了溢出状态。

        **输出：**

        - **Tuple** [object, object]，GPU后端的第一个值为False，而其他后端的第一个值是NPUAllocFloatStatus的实例。该值用于在 `get_overflow_status` 期间检测溢出。第二个值与 `compute_input` 的输入相同，用于控制执行序。
