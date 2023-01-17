#include <llapi/LoggerAPI.h>

#include <llapi/EventAPI.h>

#include "version.h"

extern Logger logger;


#include <llapi/mc/CompoundTag.hpp>
class NbtIo {
  public:
    virtual bool output(std::unique_ptr<CompoundTag> &) = 0;
    virtual std::unique_ptr<CompoundTag> input() = 0;
};

class FileNbtIo : public NbtIo
{
    public:
      enum class Type { bin, snbt };

    private:
      Type mType;
      std::string mPath;

    public:
      FileNbtIo(std::string path, Type type) { 
        mPath = path;
        mType = type;
       }
      virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
        bool r = false;
        if(mType==Type::bin)
        {
          std::fstream fout(mPath, std::ios::out | std::ios::binary | std::ios::ate);
          fout << nbt->toBinaryNBT();
          fout.close();
          r = true;
        }
        else if(mType==Type::snbt)
        {
          std::fstream fout(mPath, std::ios::out | std::ios::ate);
          fout << nbt->toSNBT();
          fout.close();
          r = true;
        }
        return r;
      }
      virtual std::unique_ptr<CompoundTag> input() {
        auto r = CompoundTag::create();
        if(mType==Type::bin)
        {
          std::string b;
          auto file = fopen(mPath.c_str(), "rb+");
          while (true)
          {
            auto c = getc(file);
            if(c==EOF)
              break;
            else
              b += (char)c;
            r = CompoundTag::fromBinaryNBT(b);
          }
        }
        else if(mType==Type::snbt)
        {
          std::fstream fin(mPath, std::ios::in);
          std::string s,t;
          while(fin>>t)
            s += t;
          r = CompoundTag::fromSNBT(s);
        }
        return std::move(r);
      }
};

class EntityNbtIo : public NbtIo 
{
  private:
    Actor *mEntity;

  public:
    EntityNbtIo(Actor *entity) { mEntity = entity; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) { 
      return nbt->setActor(mEntity);
    }
    virtual std::unique_ptr<CompoundTag> input(){
      return CompoundTag::fromActor(mEntity);
    }
};

class PlayerNbtIo : public NbtIo 
{
  private:
    Player *mPlayer;
  public:
    PlayerNbtIo(Player *player) { mPlayer = player; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
      return nbt->setPlayer(mPlayer);
    }
    virtual std::unique_ptr<CompoundTag> input() {
      return CompoundTag::fromPlayer(mPlayer);
    }
};

class ItemStackNbtIo : public NbtIo 
{
  private:
    ItemStack *mItem;
  public:
    ItemStackNbtIo(ItemStack *item) { mItem = item; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
      nbt->setItemStack(mItem);
      return true;
    }
    virtual std::unique_ptr<CompoundTag> input() {
      return CompoundTag::fromItemStack(mItem);
    }
};

class BlockNbtIo : public NbtIo 
{
  private:
    Block *mBlock;
  public:
    BlockNbtIo(Block *block) { mBlock = block; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
      nbt->setBlock(mBlock);
      return true;
    }
    virtual std::unique_ptr<CompoundTag> input() {
      return CompoundTag::fromBlock(mBlock);
    }
};

class BlockActorNbtIo : public NbtIo 
{
  private:
    BlockActor *mBlockActor;
  public:
    BlockActorNbtIo(BlockActor *blockActor) { mBlockActor = blockActor; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
      return nbt->setBlockActor(mBlockActor);
    }
    virtual std::unique_ptr<CompoundTag> input() {
      return CompoundTag::fromBlockActor(mBlockActor);
    }
};

class SnbtIo : public NbtIo 
{
  private:
    std::string mSnbt;
  public:
    SnbtIo(std::string snbt) { mSnbt = snbt; }
    virtual bool output(std::unique_ptr<CompoundTag> &nbt) {
      mSnbt = nbt->toSNBT();
      return true;
    }
    virtual std::unique_ptr<CompoundTag> input() {
      return CompoundTag::fromSNBT(mSnbt);
    }
};

#include <llapi/mc/Command.hpp>
#include <llapi/RegCommandAPI.h>

#include <llapi/mc/Dimension.hpp>
#include <llapi/mc/Level.hpp>
#include <llapi/mc/Block.hpp>
#include <llapi/mc/Container.hpp>
class NbtCommand : public Command 
{
  CommandSelector<Actor> FmActorSelector;
  CommandSelector<Player> FmPlayerSelector;
  CommandPosition FmBlockPos;
  std::string FmPath;
  std::string FmSnbt;
  int FmSlot;
  bool Fmasis, Fmpsis, Fmbpis, Fmpis, Fmsnis, Fmsis;

  CommandSelector<Actor> TmActorSelector;
  CommandSelector<Player> TmPlayerSelector;
  CommandPosition TmBlockPos;
  std::string TmPath;
  int TmSlot;
  bool Tmasis, Tmpsis, Tmbpis, Tmpis, Tmsis;

  FileNbtIo::Type FmType;
  bool Fmtis;
  FileNbtIo::Type TmType;
  bool Tmtis;

public:
  virtual void execute(class CommandOrigin const &ori,
                       class CommandOutput &output) const override {
      NbtIo *FnbtIo = nullptr;
      if(Fmasis&&!FmActorSelector.results(ori).data->empty())
      {
          auto r = FmActorSelector.results(ori);
          if(!r.empty())
          {
            auto entity = r.data->at(0);
            FnbtIo = new EntityNbtIo(entity);
          }
      }
      else if(Fmbpis)
      {
          auto block = Level::getBlock(FmBlockPos.getBlockPos(Vec3(), Vec3()),
                                       ori.getDimension()->getDimensionId());
          if(block!=nullptr)
          {
            FnbtIo = new BlockNbtIo(block);
          }
      }
      else if(Fmpis&&Fmtis)
      {
          FnbtIo = new FileNbtIo(FmPath, FmType);
      }
      else if(Fmpsis)
      {
          auto r = FmPlayerSelector.results(ori);
          if(!r.empty())
          {
            auto player = r.data->at(0);
            FnbtIo = new PlayerNbtIo(player);
          }
      }
      else if(Fmsis&&ori.getPlayer()->isPlayer()&&ori.getPlayer()->getInventorySize()>FmSlot)
      {
          ItemStack *item = ori.getPlayer()->getInventory().getSlot(FmSlot);
          FnbtIo = new ItemStackNbtIo(item);
      }
      else if(Fmsnis)
      {
          FnbtIo = new SnbtIo(FmSnbt);
      }
      else
      {
          output.error("no input");
          return;
      }
      NbtIo *TnbtIo = nullptr;
      if(Tmasis&&!TmActorSelector.results(ori).data->empty())
      {
          auto r = TmActorSelector.results(ori);
          if(!r.empty())
          {
            auto entity = r.data->at(0);
            TnbtIo = new EntityNbtIo(entity);
          }
      }
      else if(Tmbpis)
      {
          auto block = Level::getBlock(TmBlockPos.getBlockPos(0, ori, Vec3()),
                                       ori.getDimension()->getDimensionId());
          if(block!=nullptr)
          {
            TnbtIo = new BlockNbtIo(block);
          }
      }
      else if(Tmpis&&Tmtis)
      {
          TnbtIo = new FileNbtIo(FmPath, FmType);
      }
      else if(Tmpsis)
      {
          auto r = TmPlayerSelector.results(ori);
          if(!r.empty())
          {
            auto player = r.data->at(0);
            TnbtIo = new PlayerNbtIo(player);
          }
      }
      else if(Tmsis&&ori.getPlayer()->isPlayer()&&ori.getPlayer()->getInventorySize()>TmSlot)
      {
          ItemStack *item = ori.getPlayer()->getInventory().getSlot(TmSlot);
          TnbtIo = new ItemStackNbtIo(item);
      }
      else
      {
          output.addMessage(FnbtIo->input()->toSNBT());
          output.success();
          return;
      }
      auto nbt = FnbtIo->input();
      for(auto p:Global<Level>->getAllPlayers())
      {
          p->refreshInventory();
      }
      if(TnbtIo->output(nbt))
      {
          output.success();
      }
      else
      {
          output.error("unkown error");
      }
    }

  public:
    static void setup(CommandRegistry *cr) {
      using namespace RegisterCommandHelper;
      cr->registerCommand("nbt", "nbt io", CommandPermissionLevel::GameMasters,
                          CommandFlag{(CommandFlagValue)1},
                          CommandFlag{(CommandFlagValue)0});
      cr->addEnum<FileNbtIo::Type>("fileiotype", {{"bin", FileNbtIo::Type::bin},
                                              {"snbt", FileNbtIo::Type::snbt}});
      std::vector<CommandParameterData> from, to;
      from.push_back(makeOptional(&NbtCommand::FmActorSelector,
                                  std::string("from"), &NbtCommand::Fmasis));
      from.push_back(makeOptional(&NbtCommand::FmPlayerSelector,
                                  std::string("from"), &NbtCommand::Fmpsis));
      from.push_back(makeOptional(&NbtCommand::FmBlockPos,
                                  std::string("from"), &NbtCommand::Fmbpis));
      from.push_back(makeOptional(&NbtCommand::FmSnbt,
                                  std::string("from"), &NbtCommand::Fmsnis));
      from.push_back(makeOptional(&NbtCommand::FmSlot,
                                  std::string("from"), &NbtCommand::Fmsis));

      to.push_back(makeOptional(&NbtCommand::TmActorSelector,
                                  std::string("to"), &NbtCommand::Tmasis));
      to.push_back(makeOptional(&NbtCommand::TmPlayerSelector,
                                  std::string("to"), &NbtCommand::Tmpsis));
      to.push_back(makeOptional(&NbtCommand::TmBlockPos,
                                  std::string("to"), &NbtCommand::Tmbpis));
      to.push_back(makeOptional(&NbtCommand::TmSlot,
                                  std::string("to"), &NbtCommand::Tmsis));
      for(auto& fi:from)
      {
        for(auto& ti:to)
        {
            cr->registerOverload<NbtCommand>("nbt", fi, ti);
        }
      }
      auto Ftype = makeMandatory(&NbtCommand::FmType, "from", &NbtCommand::Fmtis);
      auto Fpath =
          makeOptional(&NbtCommand::FmPath, "from", &NbtCommand::Fmpis);
      auto Ttype = makeMandatory(&NbtCommand::TmType, "to", &NbtCommand::Tmtis);
      auto Tpath = makeOptional(&NbtCommand::TmPath, "to", &NbtCommand::Tmpis);
      for(auto& ti:to)
      {
        cr->registerOverload<NbtCommand>("nbt", Fpath, Ftype, ti);
      }
      for(auto& fi:from)
      {
        cr->registerOverload<NbtCommand>("nbt", fi, Tpath, Ttype);
      }
      }
};

void PluginInit()
{
  Event::RegCmdEvent::subscribe_ref([](Event::RegCmdEvent &event) {
    NbtCommand::setup(event.mCommandRegistry);
    return true;
  });
}