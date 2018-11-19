#pragma once

class SRHelper {
private:
	SRHelper() {}
	~SRHelper() {}
public:
	static SRHelper* instance()
	{
		static SRHelper helper;
		return &helper;
	}

public:
	int a, b, c;

};
