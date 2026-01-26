#include <bits/stdc++.h>

void merge(std::vector<int>& arr, int left, int mid, int right);

void mergeSort(std::vector<int>& arr, int left, int right) {
  if (left >= right) return;

  int mid = left + (right - left) / 2;

  mergeSort(arr, left, mid);
  mergeSort(arr, mid + 1, right);

  merge(arr, left, mid, right);
}

int test();

int main() {
  std::vector<int> arr = {12, 11, 13, 5, 6, 1};
  int arr_size = arr.size();

  mergeSort(arr, 0, arr_size - 1);

  return arr[0] - test();
}