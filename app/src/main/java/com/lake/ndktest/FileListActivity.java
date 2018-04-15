package com.lake.ndktest;

import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;

public class FileListActivity extends AppCompatActivity {
    private ListView listView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_list);
        listView = (ListView)findViewById(R.id.listview);
        final String folderurl = Environment.getExternalStorageDirectory().getPath();
        final File[] files = new File(folderurl + "/MyLocalPlayer").listFiles();
        Log.e("lake", "onCreate:files= "+files.length);
        MyListAdapter myListAdapter =new MyListAdapter(this,files);
        listView.setAdapter(myListAdapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                Intent intent = new Intent();
                intent.setClass(FileListActivity.this,VideoActivity.class);
                intent.putExtra("path",folderurl+ "/MyLocalPlayer/"+files[position].getName());
                intent.putExtra("name",files[position].getName());
                startActivity(intent);
            }
        });
    }

    public class MyListAdapter extends BaseAdapter{
        private Context context;
        private File[] files;

        public MyListAdapter(Context context, File[] files) {
            this.context = context;
            this.files = files;
        }

        @Override
        public int getCount() {
            return files.length;
        }

        @Override
        public Object getItem(int position) {
            return files[position];
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder viewHolder = null;
            if (convertView == null) {
                viewHolder = new ViewHolder();
                convertView = LayoutInflater.from(context).inflate(R.layout.list_item,null);
                viewHolder.mTextView = (TextView) convertView.findViewById(R.id.filename);
                convertView.setTag(viewHolder);
            }else{
                viewHolder = (ViewHolder) convertView.getTag();
            }
            viewHolder.mTextView.setText(files[position].getName());
            return convertView;
        }
        class ViewHolder{
            TextView mTextView;
        }
    }
}
