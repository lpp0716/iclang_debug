#include <bits/stdc++.h>

void merge(std::vector<int>& arr, int left, int mid, int right) {
  int n1 = mid - left + 1;
  int n2 = right - mid;

  std::vector<int> leftArr(n1), rightArr(n2);

  for (int i = 0; i < n1; ++i)
    leftArr[i] = arr[left + i];
  for (int j = 0; j < n2; ++j)
    rightArr[j] = arr[mid + 1 + j];

  int i = 0, j = 0, k = left;
  while (i < n1 && j < n2) {
    if (leftArr[i] <= rightArr[j]) {
      arr[k++] = leftArr[i++];
    } else {
      arr[k++] = rightArr[j++];
    }
  }

  while (i < n1) {
    arr[k++] = leftArr[i++];
  }
  while (j < n2) {
    arr[k++] = rightArr[j++];
  }
}

void mergeSort(std::vector<int>& arr, int left, int right) {
  if (left >= right) return;

  int mid = left + (right - left) / 2;

  mergeSort(arr, left, mid);
  mergeSort(arr, mid + 1, right);

  merge(arr, left, mid, right);
}

int test() {
  return 1;
}

int main() {
  std::vector<int> arr = {12, 11, 13, 5, 6, 1};
  int arr_size = arr.size();

  mergeSort(arr, 0, arr_size - 1);

  return arr[0] - test();
}