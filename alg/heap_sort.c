int sift_down(int a[], int start, int end)
{
	int root = start;
	for (int child = start * 2 + 1; child <= end;)
	{
		int swap = root;
		if (a[swap] < a[child]) 
			swap = child;
		child++;
		if (child <= end && a[swap] < a[child]) 
			swap = child;
		if (root == swap)
			return 0;
		int t = a[root];
		a[root] = a[swap];
		a[swap] = t;
		root = swap;
		child = root * 2 + 1;
	}
	return 0;
}

int heapify(int a[], int count)
{
	for (int root = (count - 2) / 2; root >= 0; --root)
	{
		sift_down(a, root, count - 1);
	}

	return 0;
}
int heap_sort(int a[], int count)
{
	heapify(a, count);

	for (int end = count - 1; end > 0; )
	{
		int t = a[end];
		a[end] = a[0];
		a[0] = t;
		--end;
		sift_down(a, 0, end);
	}
	return 0;
}
