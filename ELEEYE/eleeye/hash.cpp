/*
hash.h/hash.cpp - Source Code for ElephantEye, Part V

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.12, Last Modified: Dec. 2007
Copyright (C) 2004-2007 www.elephantbase.net

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include "../utility/base.h"
#include "position.h"
#include "hash.h"

int nHashMask;
HashStruct *hshItems;
#ifdef HASH_QUIESC
  HashStruct *hshItemsQ;
#endif

// �洢�û���������Ϣ
void RecordHash(const PositionStruct &pos, int nFlag, int vl, int nDepth, int mv) {
  HashStruct hsh;
  int i, nHashDepth, nMinDepth, nMinLayer;
  // �洢�û���������Ϣ�Ĺ��̰������¼������裺

  // 1. �Է�ֵ��ɱ�岽��������
  __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
  __ASSERT(mv == 0 || pos.LegalMove(mv));
  if (vl > WIN_VALUE) {
    vl += pos.nDistance;
    if (mv == 0 && vl <= BAN_VALUE) {
      return; // ���³����ľ���(�������û��ü�)���������ŷ�Ҳû�У���ôû�б�Ҫд���û���
    }
  } else if (vl < -WIN_VALUE) {
    if (mv == 0 && vl >= -BAN_VALUE) {
      return; // ͬ��
    }
    vl -= pos.nDistance;
  } else if (vl == pos.DrawValue() && mv == 0) {
    return;   // ͬ��
  }

  // 2. �����̽�û�����
  nMinDepth = 512;
  nMinLayer = 0;
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);

    // 3. �����̽��һ���ľ��棬��ô�����û�����Ϣ���ɣ�
    if (HASH_POS_EQUAL(hsh, pos)) {
      // �����ȸ�����߽߱���С�����ɸ����û�����ֵ
      if ((nFlag & HASH_ALPHA) != 0 && (hsh.ucAlphaDepth <= nDepth || hsh.swvlAlpha >= vl)) {
        hsh.ucAlphaDepth = nDepth;
        hsh.swvlAlpha = vl;
      }
      // Beta���Ҫע�⣺��Ҫ��Null-Move�Ľ�㸲�������Ľ��
      if ((nFlag & HASH_BETA) != 0 && (hsh.ucBetaDepth <= nDepth || hsh.swvlBeta <= vl) && (mv != 0 || hsh.wmv == 0)) {
        hsh.ucBetaDepth = nDepth;
        hsh.swvlBeta = vl;
      }
      // ����ŷ���ʼ�ո��ǵ�
      if (mv != 0) {
        hsh.wmv = mv;
      }
      HASH_ITEM(pos, i) = hsh;
      return;
    }

    // 4. �������һ���ľ��棬��ô��������С���û����
    nHashDepth = MAX((hsh.ucAlphaDepth == 0 ? 0 : hsh.ucAlphaDepth + 256),
        (hsh.wmv == 0 ? hsh.ucBetaDepth : hsh.ucBetaDepth + 256));
    __ASSERT(nHashDepth < 512);
    if (nHashDepth < nMinDepth) {
      nMinDepth = nHashDepth;
      nMinLayer = i;
    }
  }

  // 5. ��¼�û�����
  hsh.dwZobristLock0 = pos.zobr.dwLock0;
  hsh.dwZobristLock1 = pos.zobr.dwLock1;
  hsh.wmv = mv;
  hsh.ucAlphaDepth = hsh.ucBetaDepth = 0;
  hsh.swvlAlpha = hsh.swvlBeta = 0;
  if ((nFlag & HASH_ALPHA) != 0) {
    hsh.ucAlphaDepth = nDepth;
    hsh.swvlAlpha = vl;
  }
  if ((nFlag & HASH_BETA) != 0) {
    hsh.ucBetaDepth = nDepth;
    hsh.swvlBeta = vl;
  }
  HASH_ITEM(pos, nMinLayer) = hsh;
}

/* �жϻ�ȡ�û���Ҫ������Щ�������û����ķ�ֵ����ĸ���ͬ�������в�ͬ�Ĵ�����
 * һ�������ֵ��"WIN_VALUE"����(������"-WIN_VALUE"��"WIN_VALUE"֮�䣬��ͬ)����ֻ��ȡ�����������Ҫ��ľ��棻
 * ���������ֵ��"WIN_VALUE"��"BAN_VALUE"֮�䣬���ܻ�ȡ�û����е�ֵ(ֻ�ܻ�ȡ����ŷ������ο�)��Ŀ���Ƿ�ֹ���ڳ��������µġ��û����Ĳ��ȶ��ԡ���
 * ���������ֵ��"BAN_VALUE"���⣬���ȡ����ʱ���ؿ����������Ҫ����Ϊ��Щ�����Ѿ���֤����ɱ���ˣ�
 * �ġ������ֵ��"DrawValue()"(�ǵ�һ��������������)�����ܻ�ȡ�û����е�ֵ(ԭ����ڶ��������ͬ)��
 * ע�⣺���ڵ����������Ҫ��ɱ�岽�����е�����
 */
inline int ValueAdjust(const PositionStruct &pos, Bool &bBanNode, Bool &bMateNode, int vl) {
  bBanNode = bMateNode = FALSE;
  if (vl > WIN_VALUE) {
    if (vl <= BAN_VALUE) {
      bBanNode = TRUE;
    } else {
      bMateNode = TRUE;
      vl -= pos.nDistance;
    }
  } else if (vl < -WIN_VALUE) {
    if (vl >= -BAN_VALUE) {
      bBanNode = TRUE;
    } else {
      bMateNode = TRUE;
      vl += pos.nDistance;
    }
  } else if (vl == pos.DrawValue()) {
    bBanNode = TRUE;
  }
  return vl;
}

// �����һ���ŷ��Ƿ��ȶ��������ڼ����û����Ĳ��ȶ���
inline Bool MoveStable(PositionStruct &pos, int mv) {
  // �ж���һ���ŷ��Ƿ��ȶ��������ǣ�
  // 1. û�к����ŷ�����ٶ����ȶ��ģ�
  if (mv == 0) {
    return TRUE;
  }
  // 2. �����ŷ����ȶ��ģ�
  __ASSERT(pos.LegalMove(mv));
  if (pos.ucpcSquares[DST(mv)] != 0) {
    return TRUE;
  }
  // 3. �������û�������·��Ǩ�ƣ�ʹ��·�߳���"MAX_MOVE_NUM"����ʱӦ������ֹ·�ߣ����ٶ����ȶ��ġ�
  if (!pos.MakeMove(mv)) {
    return TRUE;
  }
  return FALSE;
}

// ������·���Ƿ��ȶ�(����ѭ��·��)�������ڼ����û����Ĳ��ȶ���
static Bool PosStable(const PositionStruct &pos, int mv) {
  HashStruct hsh;
  int i, nMoveNum;
  Bool bStable;
  // pos������·�߱仯�������ջỹԭ�����Ա���Ϊ"const"������"posMutable"�е���"const"�Ľ�ɫ
  PositionStruct &posMutable = (PositionStruct &) pos;

  __ASSERT(mv != 0);
  nMoveNum = 0;
  bStable = TRUE;
  while (!MoveStable(posMutable, mv)) {
    nMoveNum ++; // "!MoveStable()"�����Ѿ�ִ����һ���ŷ����Ժ���Ҫ����
    // ִ������ŷ����������ѭ������ô��ֹ����·�ߣ���ȷ�ϸ�·�߲��ȶ�
    if (posMutable.RepStatus() > 0) {
      bStable = FALSE;
      break;
    }
    // ����ȡ�û��������ͬ"ProbeHash()"
    for (i = 0; i < HASH_LAYERS; i ++) {
      hsh = HASH_ITEM(posMutable, i);
      if (HASH_POS_EQUAL(hsh, posMutable)) {
        break;
      }
    }
    mv = (i == HASH_LAYERS ? 0 : hsh.wmv);
  }
  // ����ǰ��ִ�й��������ŷ�
  for (i = 0; i < nMoveNum; i ++) {
    posMutable.UndoMakeMove();
  }
  return bStable;
}

// ��ȡ�û���������Ϣ(û������ʱ������"-MATE_VALUE")
int ProbeHash(const PositionStruct &pos, int vlAlpha, int vlBeta, int nDepth, Bool bNoNull, int &mv) {
  HashStruct hsh;
  int i, vl;
  Bool bBanNode, bMateNode;
  // ��ȡ�û���������Ϣ�Ĺ��̰������¼������裺

  // 1. ����ȡ�û�����
  mv = 0;
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);
    if (HASH_POS_EQUAL(hsh, pos)) {
      mv = hsh.wmv;
      __ASSERT(mv == 0 || pos.LegalMove(mv));
      break;
    }
  }
  if (i == HASH_LAYERS) {
    return -MATE_VALUE;
  }

  // 2. �ж��Ƿ����Beta�߽�
  if (hsh.ucBetaDepth > 0) {
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.swvlBeta);
    if (!bBanNode && !(hsh.wmv == 0 && bNoNull) && (hsh.ucBetaDepth >= nDepth || bMateNode) && vl >= vlBeta) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl;
      }
    }
  }

  // 3. �ж��Ƿ����Alpha�߽�
  if (hsh.ucAlphaDepth > 0) {
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.swvlAlpha);
    if (!bBanNode && (hsh.ucAlphaDepth >= nDepth || bMateNode) && vl <= vlAlpha) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl;
      }
    }
  }
  return -MATE_VALUE;
}

#ifdef HASH_QUIESC

// �洢�û���������Ϣ(��̬����)
void RecordHashQ(const PositionStruct &pos, int vlBeta, int vlAlpha) {
  volatile HashStruct *lphsh;
  __ASSERT((vlBeta > -WIN_VALUE && vlBeta < WIN_VALUE) || (vlAlpha > -WIN_VALUE && vlAlpha < WIN_VALUE));
  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask);
  lphsh->dwZobristLock0 = pos.zobr.dwLock0;
  lphsh->swvlAlpha = vlAlpha;
  lphsh->swvlBeta = vlBeta;
  lphsh->dwZobristLock1 = pos.zobr.dwLock1;
}

// ��ȡ�û���������Ϣ(��̬����)
int ProbeHashQ(const PositionStruct &pos, int vlAlpha, int vlBeta) {
  volatile HashStruct *lphsh;
  int vlHashAlpha, vlHashBeta;

  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask);
  if (lphsh->dwZobristLock0 == pos.zobr.dwLock0) {
    vlHashAlpha = lphsh->swvlAlpha;
    vlHashBeta = lphsh->swvlBeta;
    if (lphsh->dwZobristLock1 == pos.zobr.dwLock1) {
      if (vlHashBeta >= vlBeta) {
        __ASSERT(vlHashBeta > -WIN_VALUE && vlHashBeta < WIN_VALUE);
        return vlHashBeta;
      }
      if (vlHashAlpha <= vlAlpha) {
        __ASSERT(vlHashAlpha > -WIN_VALUE && vlHashAlpha < WIN_VALUE);
        return vlHashAlpha;
      }
    }
  }
  return -MATE_VALUE;
}

#endif

// UCCI֧�� - ���Hash���еľ�����Ϣ
Bool PopHash(const PositionStruct &pos) {
  HashStruct hsh;
  uint32 dwMoveStr;
  int i;

  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);
    if (HASH_POS_EQUAL(hsh, pos)) {
      printf("pophash");
      if (hsh.wmv != 0) {
        __ASSERT(pos.LegalMove(hsh.wmv));
        dwMoveStr = MOVE_COORD(hsh.wmv);
        printf(" bestmove %.4s", (const char *) &dwMoveStr);
      }
      if (hsh.ucBetaDepth > 0) {
        printf(" lowerbound %d depth %d", hsh.swvlBeta, hsh.ucBetaDepth);
      }
      if (hsh.ucAlphaDepth > 0) {
        printf(" upperbound %d depth %d", hsh.swvlAlpha, hsh.ucAlphaDepth);
      }
      printf("\n");
      fflush(stdout);
      return TRUE;
    }
  }
  return FALSE;
}