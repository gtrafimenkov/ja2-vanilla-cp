#include "Utils/Quantize.h"

#include <string.h>

#include "SGP/HImage.h"
#include "SGP/MemMan.h"

#include "SDL_pixels.h"

#define COLOUR_BITS 6
#define MAX_COLOURS 255

struct NODE {
  BOOLEAN bIsLeaf;       // TRUE if node has no children
  uint32_t nPixelCount;  // Number of pixels represented by this leaf
  uint32_t nRedSum;      // Sum of red components
  uint32_t nGreenSum;    // Sum of green components
  uint32_t nBlueSum;     // Sum of blue components
  NODE *pChild[8];       // Pointers to child nodes
  NODE *pNext;           // Pointer to next reducible node
};

static uint32_t g_leaf_count;
static NODE *g_reducible_nodes[COLOUR_BITS];

static NODE *CreateNode(const uint32_t level) {
  NODE *const node = MALLOCZ(NODE);

  node->bIsLeaf = level == COLOUR_BITS;
  if (node->bIsLeaf) {
    ++g_leaf_count;
  } else {
    node->pNext = g_reducible_nodes[level];
    g_reducible_nodes[level] = node;
  }
  return node;
}

static void AddColor(NODE **const ppNode, const uint8_t r, const uint8_t g, const uint8_t b,
                     const uint32_t level) {
  // If the node doesn't exist, create it.
  if (*ppNode == NULL) *ppNode = CreateNode(level);
  NODE *const node = *ppNode;

  if (node->bIsLeaf) {
    // Update color information if it's a leaf node.
    node->nPixelCount++;
    node->nRedSum += r;
    node->nGreenSum += g;
    node->nBlueSum += b;
  } else {
    // Recurse a level deeper if the node is not a leaf.
    const int shift = 7 - level;
    const int index = (r >> shift & 1) << 2 | (g >> shift & 1) << 1 | (b >> shift & 1);
    AddColor(&node->pChild[index], r, g, b, level + 1);
  }
}

static void ReduceTree() {
  int i;
  // Find the deepest level containing at least one reducible node.
  for (i = COLOUR_BITS - 1; i > 0 && g_reducible_nodes[i] == NULL; i--) {
  }

  // Reduce the node most recently added to the list at level i.
  NODE *const node = g_reducible_nodes[i];
  g_reducible_nodes[i] = node->pNext;

  uint32_t nRedSum = 0;
  uint32_t nGreenSum = 0;
  uint32_t nBlueSum = 0;
  uint32_t nChildren = 0;

  for (i = 0; i < 8; ++i) {
    NODE *const child = node->pChild[i];
    if (child != NULL) {
      nRedSum += child->nRedSum;
      nGreenSum += child->nGreenSum;
      nBlueSum += child->nBlueSum;
      node->nPixelCount += child->nPixelCount;
      free(child);
      node->pChild[i] = NULL;
      ++nChildren;
    }
  }

  node->bIsLeaf = TRUE;
  node->nRedSum = nRedSum;
  node->nGreenSum = nGreenSum;
  node->nBlueSum = nBlueSum;
  g_leaf_count -= nChildren - 1;
}

static NODE *ProcessImage(const SGPPaletteEntry *pData, const int iWidth, const int iHeight) {
  NODE *tree = NULL;
  for (size_t i = iWidth * iHeight; i != 0; --i) {
    SGPPaletteEntry const &c = *pData++;
    AddColor(&tree, c.r, c.g, c.b, 0);
    while (g_leaf_count > MAX_COLOURS) ReduceTree();
  }
  return tree;
}

static size_t GetPaletteColors(const NODE *const pTree, SGPPaletteEntry *const prgb, size_t index) {
  if (pTree->bIsLeaf) {
    SGPPaletteEntry *const dst = &prgb[index++];
    dst->r = pTree->nRedSum / pTree->nPixelCount;
    dst->g = pTree->nGreenSum / pTree->nPixelCount;
    dst->b = pTree->nBlueSum / pTree->nPixelCount;
    dst->a = 0;
  } else {
    for (int i = 0; i < 8; i++) {
      const NODE *const child = pTree->pChild[i];
      if (child != NULL) index = GetPaletteColors(child, prgb, index);
    }
  }
  return index;
}

static void DeleteTree(NODE *const node) {
  for (int i = 0; i < 8; i++) {
    NODE *const child = node->pChild[i];
    if (child != NULL) DeleteTree(child);
  }
  free(node);
}

static void MapPalette(uint8_t *const pDest, const SGPPaletteEntry *const pSrc,
                       const int16_t sWidth, const int16_t sHeight, const int16_t sNumColors,
                       const SGPPaletteEntry *pTable) {
  for (size_t i = sWidth * sHeight; i != 0; --i) {
    // For each palette entry, find closest
    int32_t best = 0;
    uint32_t lowest_dist = 9999999;
    for (int32_t cnt = 0; cnt < sNumColors; ++cnt) {
      const SGPPaletteEntry *const a = &pSrc[i];
      const SGPPaletteEntry *const b = &pTable[cnt];
      const int32_t dr = a->r - b->r;
      const int32_t dg = a->g - b->g;
      const int32_t db = a->b - b->b;
      const uint32_t dist = dr * dr + dg * dg + db * db;
      if (dist < lowest_dist) {
        lowest_dist = dist;
        best = cnt;
      }
    }

    // Now we have the lowest value
    // Set into dest
    pDest[i] = best;
  }
}

void QuantizeImage(uint8_t *const pDest, const SGPPaletteEntry *const pSrc, const int16_t sWidth,
                   const int16_t sHeight, SGPPaletteEntry *const pPalette) {
  // First create palette
  g_leaf_count = 0;
  FOR_EACH(NODE *, i, g_reducible_nodes) *i = 0;
  NODE *const tree = ProcessImage(pSrc, sWidth, sHeight);

  memset(pPalette, 0, sizeof(*pPalette) * 256);
  GetPaletteColors(tree, pPalette, 0);
  DeleteTree(tree);

  // Then map image to palette
  MapPalette(pDest, pSrc, sWidth, sHeight, g_leaf_count, pPalette);
}
