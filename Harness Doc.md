Table of Contents
- [1. Introduction](#1-introduction)
  - [1.1. What is "Harness"](#11-what-is-harness)
  - [1.2. Harness Module Requirement](#12-harness-module-requirement)
  - [1.3. Naming Rule](#13-naming-rule)
  - [1.4. 线束主干更新流程](#14-线束主干更新流程)
- [2. General Design](#2-general-design)
  - [2.1. Harness Environment](#21-harness-environment)
  - [2.2. Architecture](#22-architecture)
    - [2.2.1. Data flow](#221-data-flow)
    - [2.2.2. Data abstraction](#222-data-abstraction)
    - [2.2.3. Harness Brep](#223-harness-brep)
- [3. Environment](#3-environment)
  - [3.1. Harness/Sheath material](#31-harnesssheath-material)
  - [3.2. Surface Attribute](#32-surface-attribute)
  - [3.3. ZwDbHarnessAsm](#33-zwdbharnessasm)
  - [3.4. ZwDbHarnessPrt](#34-zwdbharnessprt)
    - [3.4.1. Unique name cache](#341-unique-name-cache)
    - [3.4.2. Regenerate based on dependence](#342-regenerate-based-on-dependence)
- [4. Harness standard part](#4-harness-standard-part)
  - [4.1. PIN](#41-pin)
  - [4.2. PORT](#42-port)
  - [4.3. LeadPath](#43-leadpath)
  - [4.4. Reference](#44-reference)
- [5. Harness object](#5-harness-object)
  - [5.1. Position point](#51-position-point)
  - [5.2. Segment](#52-segment)
  - [5.3. Wire](#53-wire)
  - [5.4. Cable](#54-cable)
  - [5.5. Bundle](#55-bundle)
- [6. Managers](#6-managers)
  - [6.1. Harness object manager 线束对象管理器](#61-harness-object-manager-线束对象管理器)
- [7. Model and Command](#7-model-and-command)
  - [7.1. Routing](#71-routing)
  - [7.2. Modify](#72-modify)
  - [7.3. Attach/Detach](#73-attachdetach)
  - [7.4. Delete](#74-delete)
  - [7.5. Split Segment](#75-split-segment)
  - [7.6. Create Bundle](#76-create-bundle)
- [8. Algorithm](#8-algorithm)
  - [8.1. Auto-layout algorithm](#81-auto-layout-algorithm)
  - [8.2. Cross-section layout algorithm](#82-cross-section-layout-algorithm)
  - [8.3. Smallest Enclosing Disks](#83-smallest-enclosing-disks)
  - [8.4. Topological sort](#84-topological-sort)
- [9. Mechanism](#9-mechanism)
  - [9.1. Cable pick/Hi-light switch](#91-cable-pickhi-light-switch)
  - [9.2. Hi-lighting](#92-hi-lighting)
  - [9.3. Blank/Unblank](#93-blankunblank)
  - [9.4. Regenerate](#94-regenerate)
    - [9.4.1. General regeneration mechanism](#941-general-regeneration-mechanism)
    - [9.4.2. Regenerate one harness object](#942-regenerate-one-harness-object)
  - [9.5. Through face display](#95-through-face-display)
  - [9.6. Command pre/post process](#96-command-prepost-process)
    - [9.6.1. Command reactor](#961-command-reactor)
    - [9.6.2. Reactor logic](#962-reactor-logic)
- [10. Leftover in ZW3D](#10-leftover-in-zw3d)
- [11. Things to do when ...](#11-things-to-do-when-)
  - [11.1. Add new type of object](#111-add-new-type-of-object)

- - -
# 1. Introduction
## 1.1. What is "Harness"
Harness is an assembly of electrical cables or wires which transmit signals or electrical power. 
The cables are bound together by a durable material such as rubber, electrical tape.  

## 1.2. Harness Module Requirement
Harness module is used by different type of companies in different fields.
|    Requirements    |  Status  | Version |
| :----------------: | :------: | :-----: |
|    线束材料管理    | Finished |  2730   |
|   防护层材料管理   | Finished |  2730   |
|      颜色管理      | Finished |  2730   |
|      布线命令      | Finished |  2730   |
|    布线编辑命令    | Finished |  2730   |
|     防护层创建     | Finished |  2800   |
|     防护层编辑     | Finished |  2800   |
|    依附布线命令    | Finished |  2800   |
|    脱离依附命令    | Finished |  2800   |
|   标准件类型定义   | Finished |  2730   |
|    PIN定义命令     | Finished |  2730   |
|    PORT定义命令    | Finished |  2730   |
|   钉板图生成命令   |          |  2830   |
| 钉板图自动布局算法 |          |  2830   |
|      标签定义      |          |  2XXX   |
|      布线网络      |          |  2XXX   |
|     逻辑管理器     |          |  2XXX   |
|      自动布线      |          |  2XXX   |
|    布线网络约束    |          |  2XXX   |

## 1.3. Naming Rule
线束环境中，所有在线束模块内进行声明的函数（除HarnessRoute等一些命令外）都以Hrn开头。

## 1.4. 线束主干更新流程

- DEV：Dev分支是线束集成后主干，用于向平台提交代码，修改线束bug，对线束进行跑宏验证。Beijing仓库Dev分支包括所有对应版本线束代码资源和文档。对应平台zw3d中的Dev分支为Beijing仓库中对应平台分支，命名标准为`Dev/2730_ZW3D_HARNESS` —— Dev/版本_ZW3D_HARNESS
```
// 更新zw3d Dev分支git命令

// 2731 主干更新
git checkout -b feature/Update_DevHarness2731 Dev/2731_ZW3D_HARNESS
git clean -x -f
git merge origin/Dev/2731_ZW3D -s recursive -X theirs -m 'Merge Dev/2731_ZW3D'
git revert HEAD -m 2 --no-edit
git checkout Dev/2731_ZW3D_HARNESS
git merge feature/Update_DevHarness2731 -m 'Update Dev/2731_ZW3D_HARNESS'
git push 
git branch -d feature/Update_DevHarness2731

// 2800 主干更新
git checkout -b feature/Update_DevHarness2800 Dev/2800_ZW3D_HARNESS
git clean -x -f
git merge origin/Dev/2800_ZW3D -s recursive -X theirs -m 'Merge Dev/2800_ZW3D'
git revert HEAD -m 2 --no-edit
git checkout Dev/2800_ZW3D_HARNESS
git merge feature/Update_DevHarness2800 -m 'Update Dev/2800_ZW3D_HARNESS'
git push 
git branch -d feature/Update_DevHarness2800
```
**主干更新后，需要编译确认，后期需要进行跑宏确认，然后在`每日打包`群中提交Beijing仓库的Dev主干节点hash**

- 线束开发分支：线束开发分支一般建在feature下，方面进行代码提交和交流。线束开发分支的主干更新只需要merge对应主干发过来的版本就可以，在merge过程中注意解决冲突，特别是在解决MSG文件冲突时，注意标号对应问题。

- - - 

# 2. General Design
线束的设计是基于已经设计好的模型，在已有模型上添加相关标准件，并通过wire/cable进行连接。\
<br>
实际线束设计过程中，一般情况下都是在已有模型的基础上，进行线束布线设计，而且线束设计是不会对已有模型产生任何影响的。线束相关的设计是属于基于模型的专业设计，所以在之后设计线束模块时，对线束和平台功能进行了隔离。\
<br>
同时线束设计与平台其他基于特征的设计不同。基于特征的方法所有操作是基于时序的，操作的对象是操作时模型在那个时间点的状态，并且对象的生成是基于用户输入的参数，每次重新进行生成。而线束在进行布线和编辑的过程中，线束之间是没有任何依赖关系的，线束对象之间也没有先后顺序，所以平台已有的流程和逻辑在线束模块中是不适用的。

为此我们特别设计了`基于状态`的建模流程，通过使用容器将线束流程与平台流程之间进行了隔离，线束对象在创建后将一直存在于容器中，直到对象被删除。在线束模块中将没有重生成的概念，取而代之的是刷新或更新。对象的更新时将检查依赖对象是否变更，并且通过依赖关系实现部分刷新。同时线束模块中也将没有重定义，由于对象是静态的，我们需要做的只是编辑对象中保存的数据。

线束设计建模相关技术请咨询李春梦

## 2.1. Harness Environment
在线束模块中，通过使用```ZwDbBaseObj```将线束相关的内容通过容器统一进行管理，这样线束中的各种逻辑与平台之间形成了天然的隔离。\
<br>
线束模块中，定义了两个容器，分别对应```线束装配```和```线束零件```，并且线束零件是通过虚拟零件的形式纯在的。

**线束装配**：
>线束装配的设计初衷是通过一个容器，对用户可配置数据和重用数据进行统一的定义和管理，在线束模块中，线束装配负责对`线束材料`，`防护层材料`，`线材表面属性`等进行管理。用户可以通过`导入配置文件`，或者`手动添加`的方式对材料库进行添加和维护。之后在进行线束布线设计时，就可以直接使用已经添加好的线材进行布线。这种统一配置也可以实现对不同用途线材或不同场景的布线进行分类，用户可以灵活应用。

**线束零件**：
>线束零件是线束进行实际布线的操作平台，设计人员在配置好材料库之后，通过添加一个线束零件文件的方式，进入到线束设计环境内。\
>线束零件使用`虚拟零件`进行实践，虚拟零件的使用是因为，线束零件必须依赖一个配置好的材料库才能及进行布线，并且平台中目前还没有对项目进行整体管理的机制，所以为了避免用户移动文件照成的文件关联错误或数据丢失，使用虚拟零件的方法将零件与环境进行绑定。

**具体实现**：
``` C++
class ZS_DB_API ZwDbHarnessPrt : public ZwDbBaseObj
   {
   public:
      ZwDbHarnessPrt();
      virtual ~ZwDbHarnessPrt();
   public:
      virtual int CopyTo(ZwDbBaseObj& obj) override;
      virtual int ObjIO(VsEncodeData* info) override;
      virtual int ObjMsgTrav(VsObjMsg* msg) override;
      virtual int ObjMsgMap(VsObjMsg* msg) override;
      virtual int ObjMsgPrint(VsObjMsg* msg) override { return 0; };
      virtual int ObjMsgGetName(VsObjMsg* msg) override { return 0; };
      virtual int ObjMsgCustom(VsObjMsg* msg) override { return 0; };
      virtual const char* ObjMsgClass() override;
      virtual int ObjMsg(VsObjMsg* msg) override;
   private:
      int m_version;                            /* version */
   };
```
线束装配和零件环境都是通过继承`ZwDbBaseObj`来实现的，在`ZwDbPart`中存在一个`idx_base_obj`链表，平台通过发送消息(VsObjMsg)，实现对容器内容的对象进行控制，线束模块中所有数据都是保存在线束环境对应的链表中的。具体解释详见[ZwDbHarnessAsm](#33-zwdbharnessasm)、[ZwDbHarnessPrt](#34-zwdbharnessprt)

## 2.2. Architecture
### 2.2.1. Data flow

线束的几何表达主要基于曲线，曲线来生成线束整体走向的骨架，然后通过`VxNetRodMk`实现对线束实体的建模.
- - -
**线束曲线**：

线束中一个重要的几何概念就是曲线。线束中使用的曲线是通过处理用户输入，将用户输入表达为点和线两种概念，然后通过将点和线连接起来构成组最终的曲线。

> 曲线构建流程 (HrnSegBuildCrv)：
> 1. 抽象用户输入到点和线
> 2. 调用`HrnBuildCrvFromPnts`通过`CdIcurvDefMakeNurb`将点构建成曲线
> 3. 通过使用平台`VxNrbMkCrv(V_CRV_CONCAT)`接口，将多条曲线融合成一条曲线。

用户输入主要有以下几个分类：
 - 外部数据
   - 外部引用点
   - 外部引用曲线
 - 内部数据
   - 直线，边、曲线
   - 线束布线直段功能
   - PIN、port
   - 实体上点，边，曲线

<mark>!!!NOTICE!!!<mark>
1. 当前曲线中纯在一些问题导致曲线的行为与线束的逻辑不一致
2. 曲线的编辑是基于当前曲线数据计算得到的，也就是说用户操作不同会导致不同的曲线结果，即使最终数据是完全一样的。也就是说，曲线与生成曲线的数据之间是不可逆的。
3. 线束中使用的曲线是插值曲线，每个插值点都有方向，方向的长度，G2相关信息。有个关键的概念是，插值点可以有方向但是没有长度。平台接口会根据曲线长度，计算一个方向长度放到接口中。
4. 线束默认端部必须存在方向，但是不一定有方向长度。

- - -
**线束建模**：

线束的模型在设计中存在几个特点，线束模型从逻辑上是不应该被平台已有命令作为输入，因为线束作为上层功能，所有命令的实现都必须通过线束自身的逻辑。并且平台一些操作逻辑并不适用线束模块，所以在设计线束模型时，需要考虑两者之间的隔离。

线束模型通过使用容器`ZwDbHarnessPrt`，实现了线束模型数据与平台数据的隔离，平台数据由于无法直接访问到容器内部，也就无法直接操作线束对象。

<mark>!!!BUG!!!<mark>
1. 目前线束模型生成存在问题，在中心线距离特别近或者大角度弯折重叠时，平台扫掠接口生成的面会出现破损，这个问题需要建模组跟进处理。

### 2.2.2. Data abstraction
线束的元素设计和划分是根据要完成的整体功能来决定的。首先线束需要依赖于已有模型，在已有的模型上进行布线设计，所以必然需要参考`外部几何`，用户也会在当前环境下定义一些`辅助几何`，帮助定位布线位置。在进行线缆布线调整时，可能对某点线缆经过的方向进行调整，调整方式可能接近样条曲线的编辑，所以需要提供`带方向的点`。需求中提到了需要进行`截面`管理，同时实现依附布线，两个功能都需要在一些关键位置，或者关键段上提供一定管理功能。

为了实现以上需求，线束模块抽象了一些概念
* 外部几何：所有用户定义的几何和引用进来的几何相对于线束对象来所都是外部几何。在之后的说明中，外部几何一般都指的是这个概念。
* 位置点[position point]()：一个抽象点概念，同时提供位置信息，方向信息，旋转信息。
* 线束段[segment]()：用来抽象曲线的概念，在线束中，所有线的概念都由线束段来提供。

以上几个概念都是几何概念，用来对线束模块中所有几何概念进行管理。除了几何，线束模块中还需要基本业务概念对应线束上层业务需求。
* 单芯线[wire]()：线束模块中业务级别最基本单元，提供点到点的连接，对应没有子对象的概念。
* 多芯线[cable]()：线缆概念，一般指一个对象同时管理多个子级对象。
* 线芯[core]()：在线束模块中，线芯的概念由[wire]()和[cable]()来实现，线芯既可以是最终层级，也可能是中间层级，所以线芯的概念同时符合单芯线和多芯线概念的定义。
* 防护层[bundle]()：防护层在设计中与依附布线[attach]()具有相同的概念，最简单的防护层就是在单芯线外添加一个防护层，而依附布线最基础的情况就是一条单芯线本身。一组依附布线可以在不改变任何基础连接关系和逻辑的情况下，转换为防护层状态。所以防护层相关操作和设计思路与依附布线功能是基本一致的。

线束模块中将几种概念分为了不同的层级：
> 1. 用户输入：基本用户输入
> 2. 几何： 外部引用，PIN，port，点，曲线 ... 
> 3. 几何抽象：position point，segment
> 4. 业务抽象：wire，cable，bundle
> 5. 交互逻辑：UI，command

在这个层级中，依赖关系是从1(parent)到5(child)方向构建的，高级别的对象可以参照低级别的对象，但是不能反向。同级别之间可以存在依赖关系，但是不存在循环参考关系。如果需要类似的关系，需要设计公共概念对依赖关系进行统一管理，避免任何形式的循环依赖，在[attach]()中设计了公共segment来解决多个对象之间相互依附的问题。

这个概念也决定了对象的[刷新](#94-regenerate)流程，所有线束对象的刷新都是从1-5方向进行的，一个刷新信号会根据需要，基于依赖关系向下传递。

### 2.2.3. Harness Brep

线束在设计之初是通过特征实现的，所对标对象是Creo，但是特征操作本身是不符合线束设计思路的，所以在线束模块中设计了一整套静态对象构建操作的流程。然而静态这个概念对于平台固有流程比如brep构建和显示来所可能会出现适配问题，所以为了实现brep的静态，我们在容器中构建了独属于线束的brep表达。

在平台中生成一个实体需要通过几何接口，在[toptable](https://zwiki.zwcax.com/pages/viewpage.action?pageId=20481127)中构建出实体的几何信息，包括shape、shell、face、edge等。然后在之后需要通过接口将拓扑表中的内容写入到brep DB结构中。并且在这个过程中，平台会使用VgFtrIndex获取特征的id，然后通过特征的[UID](https://zwiki.zwcax.com/display/ZW3DForum/5.Uid)构建对应几何的[永久命令](https://zwiki.zwcax.com/pages/viewpage.action?pageId=20480996)。

在实现静态对象建模的过程，需要解决的问题有两个，一个是如何保证数据保存在容器内部，一个是静态构建方法中没有特征的概念，也就没有特征id。首先线束模块中生成的所有对象都属于线束容器，线束对象创建命令在进行brep对象生成前，会通过暂存当前拓朴表，然后通过线束容器中的线束brep重构一个拓扑表进行几何生成。并且在调用几何接口之前，需要将要生成几何的线束对象id赋给VgFtrIndex，这样在生成永久命名时，几何接口和相关函数将使用线束对象的UID生成永久命名。
``` C++
int HrnObjAddToBrep(ZwDbHarnessPrt* hrnPart, int idx_hrn)
{
...
C_AUTO_REVERT_INIT(int, VgFtrIndex, idx_hrn);

ZS_GUARD(
   [&]() {CdTtClear(); pzTt = &VgTt; },
   [&]() {CdTtClear(); CdTargUpdTt(&pzTt); });

/* init harness top-table */
int idx_brep = hrnPart->getBrepIdx();
if (CdBrepAddShape(idx_brep))
   return 1;
VsObjHandle oh_brep{};
VmInitOh(oh_brep, idx_brep);
if (OmObjClassErr(&oh_brep, VBREP)) return 1;
/* generate a top-table reference to the active database */
CdTtDbNewId(OmNameGet(&VgOm->bin->name), NULL, &db_id);
/* load the specified VBREP into the active topology table */
if (CdTtFromBrep(&oh_brep, &VgTt, db_id, 0, 0))
   return 1;
VxStrCpy(VgTtBin, OmNameGet(&VgOm->bin->name), sizeof(VgTtBin));
VgTtIdx = oh_brep.index;
...
...

if (CdTtToBrep(pzTt, &oh_brep))
   return 1;
}
```
- - -
# 3. Environment
## 3.1. Harness/Sheath material
Harness material manager stores all material information used for harness routing. Users can setup a development environment for harness design. Users can also split different subjects of work by creating different harness assembly and custom different environment for them.

## 3.2. Surface Attribute
In 2730 version of harness module, there is a color DB object to bind color with a name. In future version zw3d, an appearance feature will be added. After that, this new feature will offer managements for appearance attributes. All harness material will get color information based on appearance name.

## 3.3. ZwDbHarnessAsm
线束装配环境，用于保存所有材料相关信息，其中包括各种类型属性的列表和不同材料的列表。
线束装配中提供了各种接口用来获取材料信息，在使用这类接口之前，需要通过`ZwGetHarnessAsmByPart`获取线束装配handle。

## 3.4. ZwDbHarnessPrt
线束零件环境是线束功能的主要操作平台，用户通过使用线束命令，操作线束对象实现线束设计。
线束零件环境是通过继承`ZwDbBaseObj`实现的一个容器。在这个容器中，所有对象的操作和维护都必须通过线束命令来实现。

**ZwDbBaseObj**
``` C++
class ZS_DB_API ZwDbBaseObj
{
public:
   ZwDbBaseObj();
   virtual ~ZwDbBaseObj();
public:
   virtual int ObjIO(VsEncodeData* info) = 0;
   virtual int ObjMsgTrav(VsObjMsg* msg) = 0;
   virtual int ObjMsgMap(VsObjMsg* msg) = 0;
   virtual int ObjMsgPrint(VsObjMsg* msg) = 0;
   virtual int ObjMsgGetName(VsObjMsg* msg) = 0;
   virtual const char* ObjMsgClass() = 0;
   virtual int ObjMsgCustom(VsObjMsg* msg) = 0;
   virtual int ObjMsg(VsObjMsg* msg) = 0;
   virtual int CopyTo(ZwDbBaseObj& obj) = 0;
public:
   void SetIdx(int idx);
   int GetIdx() const;
   virtual int ObjTouch();
private:
   int m_idx;
};
```
zw3d中ZwDbBaseObj通过处理平台各种消息来实现对内部信息的控制。

**ZwDbHarnessPrt**
``` C++
class ZS_DB_API ZwDbHarnessPrt : public ZwDbBaseObj
   {
   public:
      ZwDbHarnessPrt();
      virtual ~ZwDbHarnessPrt();
   public:
   ... Message Functions...
   public:
      int objAddTo(VsObjHandle *oh);
      int regenAll(void);
      int regenTargObj(std::vector<int>& targObj);
      int travHrnObj(int hrnObjType, VsObjMsg* pzMsg);
      ...
   private:
      VsApplTopTable *hrnTt;                    /* harness top-table */
      int updateRoot;                           /* flag to indicate update root
      /* environment base object */
      int idx_prt;                              /* part index */
      int idx_brep;                             /* b-rep list */
      int idx_depGraph;                         /* dependence graph */
      /* harness object */
      int idx_root;                             /* regenerate root object list */
      int idx_pospnt;                           /* position point object list */
      int idx_segment;                          /* segment object list */
      int idx_wire;                             /* wire object list  */
      int idx_cable;                            /* cable object list */
      int idx_ribbon;                           /* ribbon object list */
      int idx_bundle;                           /* bundle object list */
```
在线束零件环境中，提供了几个关键的接口，实现对线束对象的添加，删除，刷新，遍历。同时线束环境中保存了所有线束对象，未来如果扩展线束模块的功能，就需要在环境中添加对应的环境列表和处理流程。具体添加方法在[Add new type of object](#111-add-new-type-of-object)。

### 3.4.1. Unique name cache
There is a cache inside `ZwDbHarnessPrt` to keep tracking harness object names.
For all harness object, a unique name is signed to each object when the object is created. To eliminate duplicated names, every time user inputs a name. Command will check the name using this cache. 

The origin design is harness part will maintain the cache when harness objects changed. When an object is added or deleted, the cache will automatically change accordingly. However, to keep tracking the change of harness object because by the existence of `undo` and `redo`. So harness part will lose tracking when undo/redo happens. In current version, every time `ZwDbHarnessPrt::uniqueNameGen` is called, the cache will clean and rebuild.

### 3.4.2. Regenerate based on dependence
In some situations, harness design may involve thousands of wires/cables/bundles. When harness referenced target is changed, there is a potential performance issue. To avoid this problem and update object only when it is needed, we implement an update mechanism based on object dependence using [topological sort](#84-topological-sort). 

Every harness object contains two fields to record parent and child dependence information.
``` C++
typedef struct ZwDbHrnWire
   {
   double r;               /* radius of wire */
   int type;               /* type of wire (ZeHrnObjType) */
   int idx_name;           /* index of wire name VSTRING */
   int idx_seg;            /* index of segment of center line */
   int idx_mtl;            /* index of material */
   int idx_obj_parent;     /* index of list of parent dependence object */
   int idx_obj_child;      /* index of list of child dependence object */
   ...
   } ZwDbHrnWire;
```
- `idx_obj_parent` holds the objects index which current harness object will get information from.
- `idx_obj_child` holds the objects index which current harness object will offer data to.
> Check [Regenerate](#94-regenerate) for more information about regenerate mechanism.
- - -
# 4. Harness standard part
线束标准件在线束设计中提供标准复用模型，用户可以创建自己的标准件库。
线束标准件可以分为几个类别，在当前版本中分为：
1. 连接器
2. 固定器
3. 引导部件

## 4.1. PIN
PIN对应的是电气组件中的PIN脚概念，定义了每个连接器上面电线连接的位置和标记信息。

详见：[标准件-PIN](标准件.md#41-pin)

## 4.2. PORT
PORT对应两种不同的概念：
- 固定：表达式固定件或者引导部件中，线缆进入标准件的位置。
- PIN集合信息：用于对一组PIN进行管理。

详见：[标准件-PORT](标准件.md#42-port)

## 4.3. LeadPath
路径的设计是用来处理引导部件中，引导路径的创建编辑和引用的。设置路径的功能可以提供给用户更大的自定义空间。一个引导路径的端部必须绑定PORT。

详见：[标准件-LEADPATH](标准件.md#43-leadpath)


## 4.4. Reference
标准件在线束文件中进行使用，需要提前引用到当前文件中，通过PORT或者LEADPATH的reference命令，创建一个引用特征，从而实现在线束文件内，对外部标准件的使用。

详见：[标准件-reference](标准件.md#44-reference)

- - -
# 5. Harness object

## 5.1. Position point
位置点是线束模块中非常重要的对象。位置点同时提供了位置信息，方向信息，截面信息。布线，线芯创建，编辑，界面管理，寻路等等功能都需要基于位置点来实现。
```C++
int idx_seg_pos;        /* index of list of segment feature in positive side of position point */
int idx_seg_neg;        /* index of list of segment feature in negative side of position point */
int idx_seg_pos_loc;    /* coordinate of segment in positive side of position point */
int idx_seg_neg_loc;    /* coordinate of segment in negative side of position point */
```
位置点中，为了实现截面管理，需要对未来存储在位置点中的segment进行规划，计算每个segment在这个点处，相对于方向的偏移。由于cable在父级与子级交界处左右两侧segment数量是不对等的，所以在位置点中需要区分截面方向，与点方向相同一侧认为是pos，反方向为neg。

位置点根据不同的输入数据分为不同的类型：
```C++
enum ZeHrnPosPntType
    {
    V_HRN_POSPNT_DEFAULT = 0,              /* default type position point: pure xyz */
    V_HRN_POSPNT_ON_CRV = 1,               /* position point on curve */
    V_HRN_POSPNT_ON_SRF = 2,               /* position point on surface */
    V_HRN_POSPNT_ON_PIN = 3,               /* position point reference PIN */
    V_HRN_POSPNT_ON_PORT_MULTI = 4,        /* position point reference port */
    V_HRN_POSPNT_ON_PORT_FIX = 5,          /* position point reference port */
    V_HRN_POSPNT_NODE = 6,                 /* position point as routing network node */
    };
enum ZeHrnPosPntDispType
    {
    V_HRN_POSPNT_DISP_DEFAULT = 0,         /* default display type: sphere */
    V_HRN_POSPNT_DISP_DATA,                /* hide mode: not show */
    V_HRN_POSPNT_DISP_NETWORK,             /* network node mode: */
    };    
```
`ZeHrnPosPntType`确定的是位置点参照对象的类型，'ZeHrnPosPntDispType`表达的是这个位置点是单纯存储数据，还是表达完整的位置点功能。未来在线束布线网络中，位置点可以作为网络节点参与相关操作。

## 5.2. Segment
线束段用来表达线束模块中所有线的概念，只要是在线束中需要使用的线，都必须先通过一些方法，统一抽象为segment进行操作。

Segment类型：
```
enum ZeHrnSegType
    {
    V_HRN_SEG_DEFAULT = 0,           /* default segment type */
    V_HRN_SEG_LINE = 1,              /* segment for straight line */
    V_HRN_SEG_CRV = 2,               /* segment for referenced curve */
    V_HRN_SEG_ATTACH = 3,            /* segment used for center */
    V_HRN_SEG_NETWORK = 4,           /* segment used for network */
    };
```
在各种segment中，V_HRN_SEG_CRV和V_HRN_SEG_LINE类型的segment不参与刷新检查，这两个类型是用来存储几何数据的。V_HRN_SEG_ATTACH是专门用来解决依附布线问题的segment，它所参照的对象是另一个segment。
## 5.3. Wire
```C++
typedef struct ZwDbHrnWire
   {
   double r;               /* radius of wire */
   int idx_seg;            /* index of segment of center line */
   int idx_mtl;            /* index of material */
   int idx_shell;          /* index of shell to store all faces of wire */
   VsSurfAt surf_at;       /* surface display attributes */
   VsWireAt at;            /* wireframe display attributes */
   ...
   } ZwDbHrnWire;
```
线束模块中，所有需要显示表面的对象核心结构与wire基本相同。对象中都需要提供如下几种元素：
* radius+VsSurfAt：保存显示表达需要的必备数据
* idx_seg：提供中心线
* idx_mtl：获取材料信息，可以用来更新显示数据
* idx_shell：用来保存线束对象与显示数据之间的关系
* VsWireAt：用来处理Blank等显示效果

>显示流程：
wire等对象的显示流程其实比较简单，都是通过直接发送消息到idx_shell实现。

>Pick流程：
wire等对象的pick都是通过`HrnWireRayPick`方法，使用投影的形式实现的。

>点pick流程：
点pick流程需要实现`V_PNT_PICK`消息的相应，在相应函数中，可以通过检查不同的状态，通过修改`VsPickInput.pnt`达到特殊pick的效果，可以参照`CdNcurvPntPick`中相关实现。

## 5.4. Cable
在Cable对象中Pick流程有些特别，除了对当前对象进行判断，还需要对子对象进行检查，使用`VmPickTravGrp(pin, oh, msg, cable->idx_child, 1, iret, skip_next);`处理内部子对象。
## 5.5. Bundle
- - -
# 6. Managers

## 6.1. Harness object manager 线束对象管理器 

线束管理器用于对线束环境内部的对象进行管理，包括线束wire、cable、bundle、标准件等。

详见：[线束对象管理器](线束管理器.md#31-线束对象管理器-harness-object-manager)

- - -
# 7. Model and Command
## 7.1. Routing

线束布线在设计初参考了很多竞品软件的操作流程，最终根据zw3d的特点，设计了类似于样条曲线创建的形式。通过用户的点击连续输入位置信息，并在过程中提供强大的输入模式，比如直段输入，选择曲线输入，面上点输入等等。并且用户可以随时调整每个输入的参数，通过echo浏览布线的走势。我们希望用户只需要进行一次命令就可以布出接近最终结果的曲线。
详见：[线束布线](线束布线.md)

## 7.2. Modify
线束编辑命令可以通过UI界面开启，也可以直接双击要编辑的对象。

详见：[线束编辑](线束编辑.md)

## 7.3. Attach/Detach
为了实现线缆之间的跟谁效果，线束模块中设计了依附布线流程。依附布线是未来进行bundle操作和自动布线的核心功能。

详见：[依附布线](依附布线.md)

## 7.4. Delete
线束对象删除命令，用于对线束环境内，线束对象进行删除操作。

详见：[线束对象删除](对象删除.md)

## 7.5. Split Segment
线束分割和合并，用于处理防护层创建或者依附布线过程中，需要对线束segment进行分割的操作。

详见：[线束分割合并](线束分割合并.md)

## 7.6. Create Bundle
线束防护层命令，用于对防护层对象进行创建，防护层表达其内部可以有不同种类的线缆或者电线通过。防护层有不同的材料，不同的材料建模方法不同。防护层的创建于依附布线，线束分割，线缆编辑等功能都存在关系。

详见：[线束防护层创建](线束防护层.md#31-防护层创建-create-bundle)
- - -

# 8. Algorithm
## 8.1. Auto-layout algorithm

## 8.2. Cross-section layout algorithm
截面管理算法是用来将不同半径的线缆，分布到一个有限的圆形范围内，用来处理过某一个点的多条线缆分布问题。
由于线缆一般情况下都是基于圆形截面的建模，所以截面管理问题也可以等价为基于不同半径的圆形几何紧凑分布问题。

圆形分布存在多种形式，目前有算法可以实现圆形基于某点的紧凑分布，基于一定范围内的分布等等。但是在问题研究时需要注意，不同的问题描述解决方案可能不适用线束中的分布问题。比较容易搜索到的是circle packing问题，这类问题是如何使用不同半径的圆形对目标进行填充，这个算法可以解决某些线缆固定器中线缆容量的问题，但是一般状态写的依附布线分布问题无法解决。另一类是基于circle packing问题的变形，去除了边界，通过动态迭代的方法，搜索符合目标的分布形态，这类算法更符合截面管理的需求。

但是自动分布或者迭代搜索的方法可能存在一个问题，就是用户通过相同的操作可能得到不同的结果，这个问题在这类优化问题中是可能出现的，所以未来如果实现类似方法，需要关注结果的稳定性。

当前截面管理是自定义规则的方法设计的处理流程。
1. 获得所有截面半径，然后从大到小排序（A，B，C，...）。
2. 以最大线作为中心A。
3. 以A为中心，位置点中截面的X方向为基准，找到B与A外切的位置放置B，将B定义为尾部。
4. 计算放置位置方法举例，比如放置C的时候，需要基于A，B以及半径计算C放置的位置。
5. 放置时检查方法举例，比如放置D时，基于A以及C计算位置，然后使用中心A和尾部B计算是否放得下。
6. 如果A已经放不下了，则将中心定义为B，C定义为尾部，知道全部圆形分布结束。

以上方法可以保证生成的结果是稳定的，可以通过控制圆形半径排序方法，唯一确定一种分布方式。

## 8.3. Smallest Enclosing Disks
在截面管理中，我们需要根据当前分布方式计算某段依附布线线缆的最小外接圆半径。这个半径可以作为未来添加bundle时的基础半径，比如使用胶带等基于厚度的材料。

外接圆的生成效果其实取决于分布算法，基于目前自定义规则方式生成的外接圆并不是最紧凑的半径最小的外接圆。

外接圆算法当前基于的方法是：[Approximating Smallest Enclosing Disks](./paper/Approximating_Smallest_Enclosing_Disks.pdf)。

## 8.4. Topological sort

- - -

# 9. Mechanism
## 9.1. Cable pick/Hi-light switch
cable的嵌套结构和业务逻辑决定了它可以拥有不同的pick和高亮方式。

高亮类型：
1. 任何位置都高亮整体
2. 高亮选择段
3. 高亮段以及其子段

pick流程：
1. 任意位置pick最上层root cable
2. 任意位置pick当前段

为了实现上面的效果，定义了两个全局变量在ZW3D中，`VgHrnCablePickMode`和`VgHrnCableHiMode`。

`VgHrnCablePickMode`：提供选择对象pick构建方法的切换，如果是`V_HRN_CABLE_PICK_SINGLE`类型，pick对象的pickpath指向的就是当前选择对象。如果是`V_HRN_CABLE_PICK_ALL`类型，则表示pick到的是当前对象的顶级对象，如果pick到的是cable中的线芯，那pickpath指向的会是cable。

`VgHrnCableHiMode`：控制的是高亮方法，如果是`V_HRN_CABLE_HI_SINGLE`类型，高亮的就是当前pick对象。如果是`V_HRN_CABLE_HI_ALL`类型，高亮的就是当前对象以及其子对象。

基于以上两个命令进行组合就可以实现之前提出的各种效果。比如当我们要处理一个cable的线芯以及其所有子的时候，需要设置`VgHrnCablePickMode = V_HRN_CABLE_PICK_SINGLE`+`VgHrnCableHiMode = V_HRN_CABLE_HI_ALL`。选择的对象是cable中的一个core，然后给用户显示的是core所在的子树高亮。然后再配合命令中的逻辑，就可以实现对一个cable中的子分支进行操作。

## 9.2. Hi-lighting
线束对象的高亮基本基于平台流程，只是在cable中需要对高亮进行特殊处理。
```C++
static int HrnCableDisp
(
VsObjHandle* oh,  /* i: object handle */
VsObjMsg* msg     /* i: object message */
)
{
...
if (diDispSkip2(oh->index, cable->at.mode, cable->layer_id))
   {
   if (VgHrnCablePickMode == V_HRN_CABLE_PICK_ALL) return 0;
   if (OmGrpMsgIdx(cable->idx_child, msg))
      return 1;
   return 0;
   }
if (OmIdxListMsg(cable->idx_shell, msg))
   return 1;
int savedActHighLightFlag = VgCdActHighLightFlag;
ZS_SCOPEGUARD([&]() { VgCdActHighLightFlag = savedActHighLightFlag; });
if (VgCdActHighLightFlag == 50 && VgHrnCableHiMode == V_HRN_CABLE_HI_SINGLE)
   {
   /* disable hi-lighting for children */
   return 0;
   }
if (OmGrpMsgIdx(cable->idx_child, msg))
   return 1;
...
```
在cable中，需要根据`VgHrnCablePickMode`的类型，控制是否高亮，以及是否将高亮向下传到。`VgCdActHighLightFlag`标识处理的是对象高亮的方法。

## 9.3. Blank/Unblank
线束DB对象实现Blank和Unblank效果需要添加一些消息处理方法。以Position point为例：
```C++
static int HrnPosPntGenAt(VsObjHandle* oh, VsObjMsg* msg);

OmMethodSet(VHRNPOSPNT, V_GEN_AT, HrnPosPntGenAt);
OmMethodSet(VHRNPOSPNT, V_SET_MODE, CdGenericSetMode);
```
在CAD模块中，为了响应平台的Blank机制，需要添加V_BLANK处理函数。
```C++
/* cad/src/class/CdHrnPosPnt.cpp */
void CdHrnPosPntInit(void)
{
OmMethodSet(VHRNPOSPNT, V_BLANK, CdGenericBlank);
}
```
> **Developer Note**: Bad Architecture. Message should be processed in same place.

平台在处理Blank和Unblank时的流程时不统一的。有时只需要实现对Attribute中mode的修改，有时却需要处理V_BLANK消息。平台处理Blank的接口是`CdBlankObj`。如果要适配Blank还需要处理在DA工具栏中的显示管理器，相关处理流畅在`CdVisibilityList.cpp`中，需要添加全局变量和不同类型的处理流程。

## 9.4. Regenerate
### 9.4.1. General regeneration mechanism
线束模块中的对象都是静态对象，所以不存在重生成，线束对象的刷新是根据对象中保存的数据，基于自己的数据对表达进行修正。
线束对象的总体刷新方法是，首先检查线束对象所依赖的数据是否更新，如果有更新，则根据对象获取信息的流程，更新自己的信息。在完成更新后，每个线束对象都会走自己的刷新流程，用来刷新表达。
- - -
`regenAll`：
```C++
int ZwDbHarnessPrt::regenAll()
{
this->initRootList();
this->objTopSort(topSortList);
/* check if referenced geometry is changed */
for (int i = 0, iChanged = 0; i < iCntRoot; i++, iChanged = 0)
   {
   VsObjHandle oh{};
   VmInitOh(oh, rootList[i]);
   try
      {
      if (OmObjClassIdx(rootList[i]) == VHRNPOSPNT)
         {
         if (OmUpdHandle(&oh))
            return 1;
         ZS_BASE_CALL(fHrnPosPntChkGeom, iChanged, &oh);
         if(iChanged)
            changedObj.push_back(oh.index);
         }
      else
         {
         if (OmUpdHandle(&oh))
            return 1;
         ZS_BASE_CALL(fHrnSegmentChkGeom, iChanged, &oh);
         if(iChanged)
            changedObj.push_back(oh.index);
         }
      }
   catch (...)
      {
      return 1;
      }
   }
if (this->checkHrnObjMtl(changedObj)) return 1;
if (this->regenTargObj(changedObj)) return 1;
return 0;
}
```
* `regenAll`提供线束part的整体刷新，但是要注意这个刷新是基于数据变更的刷新，如果没有从`HrnXXXChkGeom`中检查到更新，那对象不会刷新。
* 线束模块基于依赖关系构建了整个刷新查询结构，通过拓扑排序可以实现只刷被影响的对象。并且线束对象中，所有对象都是基于`Position point`和`Segment`构建出来的，所以在进行更新检查时，只需要检查`Position point`和`Segment`。
* `this->initRootList()`提供了更新基础，用来收集更新检查的起始点。
* `this->objTopSort(topSortList)`拓扑排序，对线束模块中所有线束对象基于依赖关系进行拓扑排序，具体方法详见[Topological sort](#84-topological-sort)。
* `this->checkHrnObjMtl(changedObj)`用来检查是否有材料变更，如果发现材料更新，则添加对象到更新列表中。
* `this->regenTargObj`刷新函数，基于刷新列表和依赖关系拓扑排序结果，将重生成消息依次分发到下游对象中。

### 9.4.2. Regenerate one harness object
重生成消息处理：
`HrnWireRegenUtil`
```C++
int HrnWireRegenUtil
(
VsObjHandle* ohWire           /* I: wire object handle */
)
{
...
/* remove shape from b-rep */
if (CdObjVarGet(ohWire, V_HRN_SHELL, &idx_shell))
   return 1;
int idx_brep = pzHrnPrt->getBrepIdx();
C_AUTO_ARRAY(int, iCnt, shellList);
if (OmIdxListGetAll(idx_shell, &iCnt, &shellList))
   return 1;
for (int i = 0; i < iCnt; i++)
   {
   if (CdBrepDelShell(idx_brep, shellList[i]))
      return 1;
   }
/* add wire to b-rep */
if (HrnObjAddToBrep(pzHrnPrt, ohWire->index))
   return 1;
...
}
```
类似与wire这种需要shell来表达表面的对象，需要根据idx_shell获得到shell信息，然后删除shell重新构建。这样才能实现对象表达的更新。
- - -
更新检查几何：
`HrnPosPntChkGeom`

通过从线束对象中保存的参考对象里面，获取到对象需要的数据，并且根据一定的逻辑，将数据进行转化。一般情况下，这个流程与布线过程中处理用户输入流程的方法是一致的，然后对这个新的数据与保存在对象中的原始数据进行对比。如果有更新，则直接更新对象中的数据。
- - -
刷新标志：`regen`

在线束的DB对象中都存在一个标识位regen，这个标识用来记录当前对象是否需要进行更新。在一些命令中，可能需要对某个对象进行更新，但是可能对象的几何信息并没有更改，比如segment被分割成了多段，但参考的对象其实并没有变，这个时候就需要regen标识。

刷新过程中，刷新流程会收集所有标记了regen标识的对象，然后将这个对象添加到待刷新列表中。在进行刷新后，regen标识会被清除。
## 9.5. Through face display
在平台中，为了实现穿透显示，需要添加几个特殊的标记到DB中，并且需要修改平台的代码。

```C++
inline int HrnPosPntSetThruDispFlag(VsObjHandle *oh, int isThruDisp)
{
...
OBJ_GET_DATA_RETURN_1(oh, VHRNPOSPNT, pospnt);
char bitFlags = pospnt->bitFlags;
if (isThruDisp)
   VmOnBit(bitFlags, 1);
else
   VmOffBit(bitFlags, 1);
if (OmObjMod(oh, pospnt->bitFlags, bitFlags)) return 1;
...
}

inline int HrnPosPntGetThruDispFlag(const ZwDbHrnPosPnt *pospnt)
{
return VmBitIsOn(pospnt->bitFlags, 1);
}
```
* 在DB对象中，如果这个对象需要进行穿透，需要设置一个bitFlags，标记当前对象需要穿透显示。
* 在DB消息中，需要处理`V_DISP_THRU_FACE`消息。
* 在文件`src\tools\CdPick.cpp`和`src\win\CdWiPnt.cpp`中，搜索`iThruFace`，添加对应DB类型。

## 9.6. Command pre/post process
### 9.6.1. Command reactor
Harness uses command reactor `cvxCmdAction` to deal with pre/post process.
``` C++
/* bind callback function reactor */
svxCmdActionParam cmdActParam = { 0 };
cmdActParam.pfn = ZwHarnessCmdReactor;
cmdActParam.param = &gsCmdReactorStart;
cvxCmdAction(VX_CMD_START, &cmdActParam);
cmdActParam.param = &gsCmdReactorEnd;
cvxCmdAction(VX_CMD_FINISHED, &cmdActParam);
```
### 9.6.2. Reactor logic
All pre/post process logic is implemented inside.
```
int ZwHarnessCmdReactor(void* param, const char* name)
```

Take routing command `HarnessRoute` as an example.

``` C++
int ZwHarnessCmdReactor(void* param, const char* name)
{
...
if (VxStrSame(name, "!HarnessRoute"))
    {
    if (param && *(int*)param == CMD_START)
        {
        gsHarnessPickMode = VgHrnCablePickMode;
        gsHarnessHiMode = VgHrnCableHiMode;
        UiSetToolBarItemEnabled("view", 11, 0);
        UiSetToolBarItemEnabled("view", 12, 0);
        HrnCablePickAll();
        HrnCableHiAll();
        }
    else if (param && *(int*)param == CMD_FINISHED)
        {
        VgHrnCablePickMode = gsHarnessPickMode;
        VgHrnCableHiMode = gsHarnessHiMode;
        UiSetToolBarItemEnabled("view", 11, 1);
        UiSetToolBarItemEnabled("view", 12, 1);
        }
    }
...
}
```
Inside `CMD_START` code block, pre-process saves global variable and initialize with value that `HarnessRoute` command need. \
After `HarnessRoute` command succeed, `CMD_FINISHED` code block will do post-process to restore global variable and related UI.\
<mark>!!!  NOTICE  !!!</mark>\
Command reactor **DO NOT** deal with command failure. When command return with error, term-function will be called according to zw3d command routine. 
``` C++
static void HarnessRouteTerm()
{
...
if (giBlankObj)
   CdBlankObjIdx(giBlankObj, 1, 1, NULL);
VgHrnCablePickMode = gsHarnessPickMode;
VgHrnCableHiMode = gsHarnessHiMode;
UiSetToolBarItemEnabled("view", 11, 1);
UiSetToolBarItemEnabled("view", 12, 1);
...
}
```

# 10. Leftover in ZW3D
# 11. Things to do when ...
## 11.1. Add new type of object
1. Add new type of object list in 'ZwDbHarnessPrt`
2. Add new type in `ZwDbHarnessPrt::objAddTo`
3. Add pre/post process for new object create function
>You need to deal with pre/post process in command reactor and term-function.
>Check [Reactor logic](#952-reactor-logic) for detail.
3. If new object has geometry(shell) to create, check `HrnObjAddToBrep` for detail.
4. Add regeneration process for new object. Check [Regenerate one harness object](#942-regenerate-one-harness-object) for detail.

