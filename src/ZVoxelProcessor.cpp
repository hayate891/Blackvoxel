/*
 * This file is part of Blackvoxel.
 *
 * Copyright 2010-2014 Laurent Thiebaut & Olivia Merle
 *
 * Blackvoxel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Blackvoxel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * ZVoxelProcessor.cpp
 *
 *  Created on: 26 mars 2011
 *      Author: laurent
 */

#include "ZVoxelProcessor.h"
#include "SDL/SDL.h"

#ifndef Z_ZGAME_H
#  include "ZGame.h"
#endif

ULong debug_DeleteRequests = 0;

int ZVoxelProcessor::thread_func(void * Data)
{
  ZVoxelProcessor * VoxelProcessor = (ZVoxelProcessor *) Data;
  while (VoxelProcessor->ThreadContinue)
  {
    VoxelProcessor->MakeTasks();
  }

  return(0);
}


void ZVoxelProcessor::MakeTasks()
{

  // Block processing

  ZVoxelSector * Sector;
  ULong cnt;


  cnt=0;

  Timer.Start();

  // Sector Tasks like face culling.

  Sector = World->SectorList;
  while (Sector)
  {
    if (!Sector->Flag_DeletePending) MakeSectorTasks(Sector);
    Sector = Sector->GlobalList_Next;
    cnt++;
  }

  if (GameEnv->Enable_MVI) VoxelReactor.ProcessSectors( ((double)Timer.GetResult()) / 1000.0 );
  // printf("Processed: %lu Sectors\n", cnt);
  SDL_Delay(200); // 2
  Timer.End();
}

ULong RNG_z = 0;

void ZVoxelProcessor::MakeSectorTasks(ZVoxelSector * Sector)
{


  // Sector Unloading.
  //
  // Compute distance of the sector from the player position sector.
  // If sector is too far, don't process further and send sector to unloading list.

  double xdist = Sector->Pos_x - Player_Sector.x;
  double ydist = Sector->Pos_y - Player_Sector.y;
  double zdist = Sector->Pos_z - Player_Sector.z;
  double Dist  = sqrt( xdist * xdist + ydist * ydist + zdist * zdist );

  if (Dist > SectorEjectDistance && !Sector->Flag_KeepInMemory)
  {

    if(World->RequestSectorEject(Sector))
    {
      Sector->Flag_DeletePending = true;
    //printf("EjectDemand : %lx L1 Start:%lu End:%lu nEntries:%lu\n",Sector,World->SectorEjectList->debug_getstart(),World->SectorEjectList->debug_GetEnd(),World->SectorEjectList->debug_GetnEntries());
      debug_DeleteRequests++;
      // printf("EjectDemand : %lx %lu\n",Sector,debug_DeleteRequests);
    }
  } // 14

  // **************************** Sector face culling ***********************

  ULong CullingResult;

  if (Sector->PartialCulling)
  {
    CullingResult = World->SectorUpdateFaceCulling_Partial(Sector->Pos_x, Sector->Pos_y, Sector->Pos_z, Sector->PartialCulling );
    Sector->PartialCulling ^= CullingResult;
    Sector->PartialCulling &= (DRAWFACE_ABOVE | DRAWFACE_BELOW | DRAWFACE_LEFT | DRAWFACE_RIGHT | DRAWFACE_AHEAD | DRAWFACE_BEHIND);
    if (CullingResult) Sector->Flag_Render_Dirty = true;
    // printf("Cull %ld,%ld,%ld :%lx (%lx)\n", Sector->Pos_x, Sector->Pos_y, Sector->Pos_z, CullingResult, (ULong)Sector->PartialCulling);
  }

  // **************************** Egmy Scattering ****************************

  if (GameEnv->GameEventSequencer->SlotIsActivated(1))
  {
    EgmyScatter.ScatterEgmys_T1(Sector);
  } else {EgmyScatter.ResetWave();}

}
