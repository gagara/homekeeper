package com.gagara.homekeeper.model;

import android.os.Bundle;

public interface Model {

    void saveState(Bundle bundle);

    void saveState(Bundle bundle, String prefix);

    void restoreState(Bundle bundle);

    void restoreState(Bundle bundle, String prefix);

    boolean isInitialized();

}
