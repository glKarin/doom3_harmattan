######################################################################
# Automatically generated by qmake (3.0) ?? 10? 10 21:17:21 2018
######################################################################

TEMPLATE = lib
TARGET = d3xp
INCLUDEPATH += .

#CONFIG += plugin # dll

include("../doom3harmattan.pri")

QMAKE_CXXFLAGS += -fexceptions -frtti

OBJECTS_DIR = ../build/d3xp
DESTDIR = ..

DEFINES += GAME_DLL

DEFINES += _D3XP CTF

LIBS            += -L../build \
 ../build/libidlib.a

PRE_TARGETDEPS += \
 ../build/libidlib.a

# Input
HEADERS += Actor.h \
           AF.h \
           AFEntity.h \
           BrittleFracture.h \
           Camera.h \
           # EndLevel.h \
           Entity.h \
           Fx.h \
           Game.h \
           Game_local.h \
           GameEdit.h \
           Grabber.h \
           IK.h \
           Item.h \
           Light.h \
           Misc.h \
           Moveable.h \
           Mover.h \
           MultiplayerGame.h \
           Player.h \
           PlayerIcon.h \
           PlayerView.h \
           Projectile.h \
           Pvs.h \
           SecurityCamera.h \
           SmokeParticles.h \
           Sound.h \
           Target.h \
           Trigger.h \
           Weapon.h \
           WorldSpawn.h \
           ai/AAS.h \
           ai/AAS_local.h \
           ai/AI.h \
           anim/Anim.h \
           anim/Anim_Testmodel.h \
           gamesys/Class.h \
           gamesys/DebugGraph.h \
           gamesys/Event.h \
           gamesys/NoGameTypeInfo.h \
           gamesys/SaveGame.h \
           gamesys/SysCmds.h \
           gamesys/SysCvar.h \
           gamesys/TypeInfo.h \
           physics/Clip.h \
           physics/Force.h \
           physics/Force_Constant.h \
           physics/Force_Drag.h \
           physics/Force_Field.h \
           physics/Force_Grab.h \
           physics/Force_Spring.h \
           physics/Physics.h \
           physics/Physics_Actor.h \
           physics/Physics_AF.h \
           physics/Physics_Base.h \
           physics/Physics_Monster.h \
           physics/Physics_Parametric.h \
           physics/Physics_Player.h \
           physics/Physics_RigidBody.h \
           physics/Physics_Static.h \
           physics/Physics_StaticMulti.h \
           physics/Push.h \
           script/Script_Compiler.h \
           script/Script_Interpreter.h \
           script/Script_Program.h \
           script/Script_Thread.h
SOURCES += Actor.cpp \
           AF.cpp \
           AFEntity.cpp \
           BrittleFracture.cpp \
           Camera.cpp \
           # EndLevel.cpp \
           Entity.cpp \
           Fx.cpp \
           Game_local.cpp \
           Game_network.cpp \
           GameEdit.cpp \
           Grabber.cpp \
           IK.cpp \
           Item.cpp \
           Light.cpp \
           Misc.cpp \
           Moveable.cpp \
           Mover.cpp \
           MultiplayerGame.cpp \
           Player.cpp \
           PlayerIcon.cpp \
           PlayerView.cpp \
           Projectile.cpp \
           Pvs.cpp \
           SecurityCamera.cpp \
           SmokeParticles.cpp \
           Sound.cpp \
           Target.cpp \
           Trigger.cpp \
           Weapon.cpp \
           WorldSpawn.cpp \
           ai/AAS.cpp \
           ai/AAS_debug.cpp \
           ai/AAS_pathing.cpp \
           ai/AAS_routing.cpp \
           ai/AI.cpp \
           ai/AI_events.cpp \
           ai/AI_pathing.cpp \
           ai/AI_Vagary.cpp \
           anim/Anim.cpp \
           anim/Anim_Blend.cpp \
           anim/Anim_Import.cpp \
           anim/Anim_Testmodel.cpp \
           gamesys/Callbacks.cpp \
           gamesys/Class.cpp \
           gamesys/DebugGraph.cpp \
           gamesys/Event.cpp \
           gamesys/SaveGame.cpp \
           gamesys/SysCmds.cpp \
           gamesys/SysCvar.cpp \
           gamesys/TypeInfo.cpp \
           physics/Clip.cpp \
           physics/Force.cpp \
           physics/Force_Constant.cpp \
           physics/Force_Drag.cpp \
           physics/Force_Field.cpp \
           physics/Force_Grab.cpp \
           physics/Force_Spring.cpp \
           physics/Physics.cpp \
           physics/Physics_Actor.cpp \
           physics/Physics_AF.cpp \
           physics/Physics_Base.cpp \
           physics/Physics_Monster.cpp \
           physics/Physics_Parametric.cpp \
           physics/Physics_Player.cpp \
           physics/Physics_RigidBody.cpp \
           physics/Physics_Static.cpp \
           physics/Physics_StaticMulti.cpp \
           physics/Push.cpp \
           script/Script_Compiler.cpp \
           script/Script_Interpreter.cpp \
           script/Script_Program.cpp \
           script/Script_Thread.cpp